/*
 * Copyright 2022 Luis Henrique O. Rios
 *
 * This file is part of MyOS.
 *
 * MyOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MyOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MyOS. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <myos.h>

#include "kernel/busy_waiting_manager.h"
#include "kernel/cmos.h"
#include "kernel/error_handler.h"
#include "kernel/log.h"
#include "kernel/mbr.h"
#include "kernel/x86.h"

#include "kernel/file_system/devices_file_system.h"

#include "kernel/io/block_device.h"
#include "kernel/io/open_file_description.h"

#include "util/math_utils.h"
#include "util/string_utils.h"
#include "util/string_stream_writer.h"

#define BYTES_PER_SECTOR 512 /* 3.1.44 sector: A uniquely addressable set of 256 words (512 bytes). */

#define IDENTIFY_DEVICE_COMMAND_CODE 0xEC
#define READ_SECTORS_COMMAND_CODE 0x20
#define WRITE_SECTORS_COMMAND_CODE 0x30

#define ERROR_REGISTER 0x01
#define SECTOR_COUNT_REGISTER 0x02
#define LBA_LOW_REGISTER 0x03
#define LBA_MIDDLE_REGISTER 0x04
#define LBA_HIGH_REGISTER 0x05
#define DEVICE_REGISTER 0x06
#define STATUS_REGISTER 0x07
#define COMMAND_REGISTER 0x07

#define ALTERNATE_STATUS_REGISTER 0x0
#define DEVICE_CONTROL_REGISTER 0x00

#define STATUS_REGISTER_BSY_MASK 0x80 /* Busy */
#define STATUS_REGISTER_DRDY_MASK 0x40 /* Device ready */
#define STATUS_REGISTER_DF_MASK 0x20 /* Device Fault */
#define STATUS_REGISTER_DRQ_MASK 0x08 /* Data request */
#define STATUS_REGISTER_ERR_MASK 0x01 /* Error */

enum ATADeviceType {
	PATA_DEVICE_TYPE = 0,
	PATAPI_DEVICE_TYPE = 1,
	SATA_DEVICE_TYPE = 2,
	SATAPI_DEVICE_TYPE = 3,
	UNKNOWN_DEVICE_TYPE = 4,
};

const char* ATA_DEVICE_TYPE_NAMES[] = {
	"PATA",
	"PATAPI",
	"SATA",
	"SATAPI",
	"UNKNOWN"
};

#define MODEL_LENGTH 40

struct ATAIdentifyReturn {
	uint16_t flags;
	uint16_t unused1[9];
	char serial[20];
	uint16_t unused2[3];
	char firmwareRevision[8];
	char model[MODEL_LENGTH];
	uint16_t unused3[2];
	uint16_t capabilities[2]; /* words: 49 - 50 */
	uint16_t unused4[2];
	uint16_t extendedInformationValidity;
	uint16_t unused5[6];
	uint32_t maxLBAAddresss28; /* words: 60 - 61 */
	uint16_t unused6[20];
	uint16_t commandSetsSupported[2]; /* words: 82 - 83 */
	uint16_t unused7[1];
	uint16_t commandSetsEnabled[2]; /* words: 85 - 86 */
	uint16_t unused8[13];
	uint64_t maxLBAAddresss48; /* words: 100 - 103 */
	uint16_t unused9[152];
} __attribute__((packed));
_Static_assert(sizeof(struct ATAIdentifyReturn) == BYTES_PER_SECTOR, "Expecting ATAIdentifyReturn with 512 bytes.");

#define ATA_DEVICE_PARTITIONS_ARRAY_LENGTH MBR_PARTITION_TABLE_LENGTH

struct ATADevicePartition {
	struct ATADevice* ataDevice;
	struct MBRPartitionTableEntry* partitionTableEntry;
};

struct ATADevice {
	bool operational;
	struct IDEChannel* ideChannel;
	bool master;
	uint8_t deviceType;
	uint8_t id;
	struct ATAIdentifyReturn ataIdentifyReturn;
	char model[MODEL_LENGTH + 1];
	struct MBR mbr;
	struct ATADevicePartition partitions[ATA_DEVICE_PARTITIONS_ARRAY_LENGTH];
	uint32_t partitionCount;
};

struct IDEChannel {
	uint16_t controlRegistersBase;
	uint16_t commandsRegistersBase;
	struct ATADevice masterATADevice;
	struct ATADevice slaveATADevice;
	uint8_t id;
	uint8_t lastStatusRegisterContent;
};

static struct IDEChannel primaryIDEChannel;
static struct IDEChannel secondaryIDEChannel;

struct ATADeviceVirtualFileSystemNode {
	struct VirtualFileSystemNode virtualFileSystemNode;
	mode_t mode;
	struct ATADevice* ataDevice;
};
struct VirtualFileSystemOperations deviceOperations;

struct ATADevicePartitionVirtualFileSystemNode {
	struct BlockDeviceVirtualFileSystemNode blockDeviceVirtualFileSystemNode;
	struct BlockDevice blockDevice;
	struct ATADevice* ataDevice;
	uint64_t firstSectorId;
	uint64_t numberOfSectors;
	uint8_t id;
	mode_t mode;
};
struct VirtualFileSystemOperations devicePartitionOperations;

#define NUMBER_OF_IDE_CHANNELS 2
#define ATA_DEVICES_PER_IDE_CHANNEL 2
#define NUMBER_OF_ATA_DEVICES (NUMBER_OF_IDE_CHANNELS * ATA_DEVICES_PER_IDE_CHANNEL)
static struct ATADeviceVirtualFileSystemNode ataDevicesVirtualFileSystemNodes[NUMBER_OF_ATA_DEVICES];
static struct ATADevicePartitionVirtualFileSystemNode ataDevicesPartitionsVirtualFileSystemNodes[NUMBER_OF_ATA_DEVICES * MBR_PARTITION_TABLE_LENGTH];

static void wait400Nanoseconds(struct IDEChannel* ideChannel) {
	x86InputByteFromPort(ideChannel->controlRegistersBase + ALTERNATE_STATUS_REGISTER);
	x86InputByteFromPort(ideChannel->controlRegistersBase + ALTERNATE_STATUS_REGISTER);
	x86InputByteFromPort(ideChannel->controlRegistersBase + ALTERNATE_STATUS_REGISTER);
	x86InputByteFromPort(ideChannel->controlRegistersBase + ALTERNATE_STATUS_REGISTER);
}

enum ATAChannelWaitResult {
	ATA_WAIT_SUCCESS,
	ATA_WAIT_TIMEOUT,
	ATA_WAIT_ERROR
};
static enum ATAChannelWaitResult waitWhileNotDone(struct IDEChannel* ideChannel, uint32_t milliseconds) {
	bool hasTimeLeft = true;
	uint64_t before = 0;
	if (milliseconds > 0) {
		before = x86Rdtsc();
	}
	uint8_t status;
	do {
		status = x86InputByteFromPort(ideChannel->commandsRegistersBase + STATUS_REGISTER);
		ideChannel->lastStatusRegisterContent = status;
	} while ((status & STATUS_REGISTER_BSY_MASK)
			&& !(status & STATUS_REGISTER_DF_MASK)
			&& !(status & STATUS_REGISTER_ERR_MASK)
			&& (milliseconds == 0 || (hasTimeLeft = busyWaitingHasTimeLeft(before, milliseconds))));

	if (!hasTimeLeft) {
		return ATA_WAIT_TIMEOUT;
	} else if ((status & STATUS_REGISTER_ERR_MASK) || (status & STATUS_REGISTER_DF_MASK)) {
		return ATA_WAIT_ERROR;
	} else {
		return ATA_WAIT_SUCCESS;
	}
}

static bool ataSoftwareReset(struct IDEChannel* ideChannel) {
	logDebug("  trying to reset IDE channel %d", ideChannel->id);

	/* It also disables the interrupt. */
	x86OutputByteToPort(ideChannel->controlRegistersBase + DEVICE_CONTROL_REGISTER, 0x06); /* It turns on the SRST bit. */
	wait400Nanoseconds(ideChannel);
	x86OutputByteToPort(ideChannel->controlRegistersBase + DEVICE_CONTROL_REGISTER, 0x02); /* It turns off the SRST bit. */
	return waitWhileNotDone(ideChannel, 1500) == ATA_WAIT_SUCCESS;
}

static enum ATAChannelWaitResult readSectors(struct ATADevice* ataDevice, uint64_t sectorId, uint16_t* buffer, size_t sectorCount) {
	struct IDEChannel* ideChannel = ataDevice->ideChannel;

	sectorId = sectorId & 0x0FFFFFFF;

	uint8_t bits27To24 = (sectorId >> 24) & 0x0F;
	uint8_t value = 0xE0 | ((ataDevice->master ? 0 : 1) << 4) | bits27To24;
	x86OutputByteToPort(ideChannel->commandsRegistersBase + DEVICE_REGISTER, value);
	wait400Nanoseconds(ideChannel);

	x86OutputByteToPort(ideChannel->commandsRegistersBase + SECTOR_COUNT_REGISTER, (uint8_t) sectorCount);

	value = sectorId & 0xFF;
	x86OutputByteToPort(ideChannel->commandsRegistersBase + LBA_LOW_REGISTER, value);

	value = (sectorId >> 8) & 0xFF;
	x86OutputByteToPort(ideChannel->commandsRegistersBase + LBA_MIDDLE_REGISTER, value);

	value = (sectorId >> 16) & 0xFF;
	x86OutputByteToPort(ideChannel->commandsRegistersBase + LBA_HIGH_REGISTER, value);

	x86OutputByteToPort(ideChannel->commandsRegistersBase + COMMAND_REGISTER, READ_SECTORS_COMMAND_CODE);
	wait400Nanoseconds(ideChannel);

	if (sectorCount == 0) {
		sectorCount = 256;
	}

	enum ATAChannelWaitResult result;
	for (int i = 0; i < sectorCount; i++) {
		result = waitWhileNotDone(ideChannel, 750);
		if (result != ATA_WAIT_SUCCESS) {
			break;
		}
		for (int j = 0; j < BYTES_PER_SECTOR / sizeof(uint16_t); j++) {
			buffer[i * (BYTES_PER_SECTOR / sizeof(uint16_t)) + j] = x86InputWordFromPort(ideChannel->commandsRegistersBase);
		}
	}

	if (result != ATA_WAIT_SUCCESS) {
		errorHandlerFatalError("There was a fatal error while reading from an ATA device");
	}

	return result;
}

static enum ATAChannelWaitResult writeSectors(struct ATADevice* ataDevice, uint64_t sectorId, uint16_t* buffer, size_t sectorCount) {
	struct IDEChannel* ideChannel = ataDevice->ideChannel;

	sectorId = sectorId & 0x0FFFFFFF;

	uint8_t bits27To24 = (sectorId >> 24) & 0x0F;
	uint8_t value = 0xE0 | ((ataDevice->master ? 0 : 1) << 4) | bits27To24;
	x86OutputByteToPort(ideChannel->commandsRegistersBase + DEVICE_REGISTER, value);
	wait400Nanoseconds(ideChannel);

	x86OutputByteToPort(ideChannel->commandsRegistersBase + SECTOR_COUNT_REGISTER, (uint8_t) sectorCount);

	value = sectorId & 0xFF;
	x86OutputByteToPort(ideChannel->commandsRegistersBase + LBA_LOW_REGISTER, value);

	value = (sectorId >> 8) & 0xFF;
	x86OutputByteToPort(ideChannel->commandsRegistersBase + LBA_MIDDLE_REGISTER, value);

	value = (sectorId >> 16) & 0xFF;
	x86OutputByteToPort(ideChannel->commandsRegistersBase + LBA_HIGH_REGISTER, value);

	x86OutputByteToPort(ideChannel->commandsRegistersBase + COMMAND_REGISTER, WRITE_SECTORS_COMMAND_CODE);
	wait400Nanoseconds(ideChannel);

	if (sectorCount == 0) {
		sectorCount = 256;
	}

	enum ATAChannelWaitResult result;
	for (int i = 0; i < sectorCount; i++) {
		/* Is it the first iteration? */
		if (i == 0) {
			result = waitWhileNotDone(ideChannel, 750);
			if (result != ATA_WAIT_SUCCESS) {
				break;
			}
		}
		for (int j = 0; j < BYTES_PER_SECTOR / sizeof(uint16_t); j++) {
			x86OutpuWordFromPort(ideChannel->commandsRegistersBase, buffer[i * (BYTES_PER_SECTOR / sizeof(uint16_t)) + j]);
		}
		result = waitWhileNotDone(ideChannel, 750);
		if (result != ATA_WAIT_SUCCESS) {
			break;
		}
	}

	if (result != ATA_WAIT_SUCCESS) {
		logDebug("id=%d sectorId=%llX sectorCount=%d", ataDevice->id, sectorId, sectorCount);
		errorHandlerFatalError("There was a fatal error while writing to an ATA device");
	}

	return result;
}

static bool canUse48BitAddressFeature(struct ATAIdentifyReturn* ataIdentifyReturn) {
	return (ataIdentifyReturn->commandSetsSupported[1] & (1 << 10)) && (ataIdentifyReturn->commandSetsEnabled[1] & (1 << 10));
}

static bool isRemovableMediaDevice(struct ATAIdentifyReturn* ataIdentifyReturn) {
	return ataIdentifyReturn->flags & (1 << 7);
}

static void initializeDevicePartitions(struct ATADevice* ataDevice) {
	logDebug("    initializing device partitions:");

	readSectors(ataDevice, 0, (uint16_t*)&ataDevice->mbr, 1);

	if (ataDevice->mbr.signature == MBR_SIGNATURE) {
		logDebug("      found a valid MBR");
		for (int i = 0; i < MBR_PARTITION_TABLE_LENGTH; i++) {
			struct MBRPartitionTableEntry* partitionTableEntry = &ataDevice->mbr.partitionTable[i];
			if (mbrIsPartitionValid(partitionTableEntry) && mbrIsPartitionTypeRecognizable(partitionTableEntry)) {
				struct ATADevicePartition* ataDevicePartition = &ataDevice->partitions[ataDevice->partitionCount++];
				ataDevicePartition->ataDevice = ataDevice;
				ataDevicePartition->partitionTableEntry = partitionTableEntry;
			}
		}

	} else {
		logDebug("      no MBR found");
	}
}

static void initializeDevice(struct ATADevice* ataDevice) {
	struct IDEChannel* ideChannel = ataDevice->ideChannel;
	ataDevice->operational = false;
	ataDevice->deviceType = UNKNOWN_DEVICE_TYPE;

	logDebug("  trying to initialize ATA device (%s at IDE channel %d):", ataDevice->master ? "master" : "slave", ataDevice->ideChannel->id);

	x86OutputByteToPort(ideChannel->commandsRegistersBase + DEVICE_REGISTER, 0xA0 | ((ataDevice->master ? 0 : 1) << 4));
	wait400Nanoseconds(ideChannel);

	/*
	 * A device shall place the signature in the Command Block registers for power-on reset, hardware reset, software reset, and the EXECUTE DEVICE
	 * DIAGNOSTIC command.
	 *
	 * Get the "signature bytes".
	 */
	uint8_t sectorCount = x86InputByteFromPort(ideChannel->commandsRegistersBase + SECTOR_COUNT_REGISTER);
	uint8_t lbaLow = x86InputByteFromPort(ideChannel->commandsRegistersBase + LBA_LOW_REGISTER);
	uint8_t lbaMiddle = x86InputByteFromPort(ideChannel->commandsRegistersBase + LBA_MIDDLE_REGISTER);
	uint8_t lbaHigh = x86InputByteFromPort(ideChannel->commandsRegistersBase + LBA_HIGH_REGISTER);
	uint8_t device = x86InputByteFromPort(ideChannel->commandsRegistersBase + DEVICE_REGISTER);

	x86OutputByteToPort(ideChannel->commandsRegistersBase + COMMAND_REGISTER, IDENTIFY_DEVICE_COMMAND_CODE);
	wait400Nanoseconds(ideChannel);

	if (x86InputByteFromPort(ideChannel->commandsRegistersBase + STATUS_REGISTER) != 0 && waitWhileNotDone(ideChannel, 500) == ATA_WAIT_SUCCESS) {
		/*
		 * Information Technology - AT Attachment with Packet Interface - 6
		 * (ATA/ATAPI-6)
		 * 9.12 Signature and persistence
		 */
		uint8_t deviceType;
		if (sectorCount == 0x01 && lbaLow == 0x01 && lbaMiddle == 0x00 && lbaHigh == 0x00 && (device & ~0xB0) == 0x00) {
			deviceType = PATA_DEVICE_TYPE;

		} else if (sectorCount == 0x01 && lbaLow == 0x01 && lbaMiddle == 0x14 && lbaHigh == 0xEB && (device & ~0xB0) == 0x00) {
			deviceType = PATAPI_DEVICE_TYPE;

		} else if (sectorCount == 0x01 && lbaLow == 0x01 && lbaMiddle == 0x3C && lbaHigh == 0xC3 && (device & ~0xB0) == 0x00) {
			deviceType = SATA_DEVICE_TYPE;

		} else if (sectorCount == 0x01 && lbaLow == 0x01 && lbaMiddle == 0x69 && lbaHigh == 0x96 && (device & ~0xB0) == 0x00) {
			deviceType = SATAPI_DEVICE_TYPE;

		} else {
			deviceType = UNKNOWN_DEVICE_TYPE;
		}

		logDebug("    \"%s\" device found", ATA_DEVICE_TYPE_NAMES[deviceType]);

		if (deviceType == PATA_DEVICE_TYPE || deviceType == SATA_DEVICE_TYPE) {
			struct ATAIdentifyReturn* ataIdentifyReturn = &ataDevice->ataIdentifyReturn;
			uint16_t *buffer = (uint16_t*) ataIdentifyReturn;

			for (int i = 0; i < BYTES_PER_SECTOR / sizeof(uint16_t); i++) {
				buffer[i] = x86InputWordFromPort(ideChannel->commandsRegistersBase);
			}

			if (!isRemovableMediaDevice(ataIdentifyReturn)) {
				for (int i = 0; i + 1 < MODEL_LENGTH; i+=2) {
					ataDevice->model[i] = ataIdentifyReturn->model[i + 1];
					ataDevice->model[i + 1] = ataIdentifyReturn->model[i];
				}
				ataDevice->model[MODEL_LENGTH] = '\0';
				stringUtilsRightTrim(ataDevice->model);


				ataDevice->operational = true;
				ataDevice->deviceType = deviceType;

				initializeDevicePartitions(ataDevice);
			}
		}

	} else {
		logDebug("    nothing found");
	}
}

static void initializeDevices(struct IDEChannel* ideChannel) {
	ideChannel->masterATADevice.ideChannel = ideChannel;
	ideChannel->masterATADevice.master = true;
	ideChannel->masterATADevice.id = ideChannel->id * ATA_DEVICES_PER_IDE_CHANNEL;

	ideChannel->slaveATADevice.ideChannel = ideChannel;
	ideChannel->slaveATADevice.master = false;
	ideChannel->slaveATADevice.id = ideChannel->id * ATA_DEVICES_PER_IDE_CHANNEL + 1;

	if (ataSoftwareReset(ideChannel)) {
		initializeDevice(&ideChannel->masterATADevice);
		initializeDevice(&ideChannel->slaveATADevice);
	} else {
		logDebug("  timeout while trying to reset the IDE channel %d", ideChannel->id);
		ideChannel->masterATADevice.operational = false;
		ideChannel->slaveATADevice.operational = false;
	}
}

static mode_t deviceGetMode(struct VirtualFileSystemNode* virtualFileSystemNode) {
	struct ATADeviceVirtualFileSystemNode* ataDeviceVirtualFileSystemNode = (void*) virtualFileSystemNode;
	return ataDeviceVirtualFileSystemNode->mode;
}

static APIStatusCode deviceOpen(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription** openFileDescription, int flags) {
	return SUCCESS;
}

static mode_t devicePartitionGetMode(struct VirtualFileSystemNode* virtualFileSystemNode) {
	struct ATADevicePartitionVirtualFileSystemNode* ataDevicePartitionVirtualFileSystemNode = (void*) virtualFileSystemNode;
	return ataDevicePartitionVirtualFileSystemNode->mode;
}

static APIStatusCode devicePartitionOpen(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription** openFileDescription, int flags) {
	return SUCCESS;
}

static APIStatusCode deviceStatus(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, struct stat* statInstance) {
	struct ATADeviceVirtualFileSystemNode* ataDeviceVirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct ATADevice* ataDevice = ataDeviceVirtualFileSystemNode->ataDevice;

	statInstance->st_dev = ATA_DEVICE_ID;
	statInstance->st_ino = ataDevice->id;
	statInstance->st_atime = cmosGetInitializationTime();
	statInstance->st_ctime = cmosGetInitializationTime();
	statInstance->st_mtime = cmosGetInitializationTime();
	statInstance->st_rdev = myosCalculateUniqueId(statInstance->st_dev, statInstance->st_ino);
	statInstance->st_blksize = BYTES_PER_SECTOR; /* As it uses only PIO Mode. */
	statInstance->st_nlink = 1;

	return SUCCESS;
}

static APIStatusCode devicePartitionStatus(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, struct stat* statInstance) {
	struct ATADevicePartitionVirtualFileSystemNode* ataDevicePartitionVirtualFileSystemNode = (void*) virtualFileSystemNode;

	statInstance->st_dev = ATA_DEVICE_PARTITION_ID;
	statInstance->st_ino = ataDevicePartitionVirtualFileSystemNode->id;
	statInstance->st_atime = cmosGetInitializationTime();
	statInstance->st_ctime = cmosGetInitializationTime();
	statInstance->st_mtime = cmosGetInitializationTime();
	statInstance->st_rdev = myosCalculateUniqueId(statInstance->st_dev, statInstance->st_ino);
	statInstance->st_blksize = BYTES_PER_SECTOR; /* As it uses only PIO Mode. */
	statInstance->st_nlink = 1;

	return SUCCESS;
}

static APIStatusCode commonRead(struct OpenFileDescription* openFileDescription, void* buffer, size_t bufferSize, size_t* count, struct ATADevice* ataDevice, uint64_t firstSectorId, uint64_t sectorCount) {
	APIStatusCode result = SUCCESS;

	if (bufferSize > 0) {
		size_t totalBytesCopied = 0;

		while (bufferSize > totalBytesCopied) {
			uint16_t ataReadBuffer[BYTES_PER_SECTOR / sizeof(uint16_t)];
			uint64_t sectorId = firstSectorId + openFileDescription->offset / BYTES_PER_SECTOR;

			if (sectorId >= firstSectorId + sectorCount) {
				break;

			} else {
				if (readSectors(ataDevice, sectorId, ataReadBuffer, 1) == ATA_WAIT_SUCCESS) {
					size_t bytesToCopy = mathUtilsMin(BYTES_PER_SECTOR - (openFileDescription->offset % BYTES_PER_SECTOR), bufferSize - totalBytesCopied);
					memcpy(buffer + totalBytesCopied, ((void*) ataReadBuffer) + (openFileDescription->offset % BYTES_PER_SECTOR), bytesToCopy);
					totalBytesCopied += bytesToCopy;
					openFileDescription->offset += bytesToCopy;

 				} else {
 					result = EIO;
 					break;
 				}
			}
		}

		*count = totalBytesCopied;
	}

	return result;
}

static APIStatusCode commonWrite(struct OpenFileDescription* openFileDescription, void* buffer, size_t bufferSize, struct ATADevice* ataDevice, uint64_t firstSectorId, uint64_t sectorCount, size_t* count) {
	APIStatusCode result = SUCCESS;

	if (openFileDescription->offset % BYTES_PER_SECTOR == 0 && bufferSize % BYTES_PER_SECTOR == 0) {
		if (bufferSize > 0) {
			size_t totalBytesCopied = 0;

			while (bufferSize > totalBytesCopied) {
				uint64_t sectorId = firstSectorId + openFileDescription->offset / BYTES_PER_SECTOR;

				if (sectorId >= firstSectorId + sectorCount) {
					break;

				} else {
					if (writeSectors(ataDevice, sectorId, buffer + totalBytesCopied, 1) == ATA_WAIT_SUCCESS) {
						totalBytesCopied += BYTES_PER_SECTOR;
						openFileDescription->offset += BYTES_PER_SECTOR;
						if (count != NULL) {
							*count += BYTES_PER_SECTOR;
						}
					} else {
						result = EIO;
						break;
					}
				}
			}
		}

	} else {
		result = EPERM;
	}

	return result;
}

static APIStatusCode deviceRead(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, void* buffer, size_t bufferSize, size_t* count) {
	struct ATADeviceVirtualFileSystemNode* ataDeviceVirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct ATADevice* ataDevice = ataDeviceVirtualFileSystemNode->ataDevice;
	struct ATAIdentifyReturn* ataIdentifyReturn = &ataDevice->ataIdentifyReturn;

	return commonRead(openFileDescription, buffer, bufferSize, count, ataDevice, 0, ataIdentifyReturn->maxLBAAddresss28);
}

static APIStatusCode deviceWrite(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, void* buffer, size_t bufferSize, size_t* count) {
	struct ATADeviceVirtualFileSystemNode* ataDeviceVirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct ATADevice* ataDevice = ataDeviceVirtualFileSystemNode->ataDevice;
	struct ATAIdentifyReturn* ataIdentifyReturn = &ataDevice->ataIdentifyReturn;

	return commonWrite(openFileDescription, buffer, bufferSize, ataDevice, 0, ataIdentifyReturn->maxLBAAddresss28, count);
}

static APIStatusCode devicePartitionRead(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, void* buffer, size_t bufferSize, size_t* count) {
	struct ATADevicePartitionVirtualFileSystemNode* ataDevicePartitionVirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct ATADevice* ataDevice = ataDevicePartitionVirtualFileSystemNode->ataDevice;

	return commonRead(openFileDescription, buffer, bufferSize, count, ataDevice, ataDevicePartitionVirtualFileSystemNode->firstSectorId, ataDevicePartitionVirtualFileSystemNode->numberOfSectors);
}

static enum OpenFileDescriptionOffsetRepositionPolicy devicegetOpenFileDescriptionOffsetRepositionPolicy(struct VirtualFileSystemNode* virtualFileSystemNode) {
	struct ATADeviceVirtualFileSystemNode* ataDeviceVirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct ATADevice* ataDevice = ataDeviceVirtualFileSystemNode->ataDevice;
	struct ATAIdentifyReturn* ataIdentifyReturn = &ataDevice->ataIdentifyReturn;
	int64_t fileSize = ((int64_t) ataIdentifyReturn->maxLBAAddresss28) * BYTES_PER_SECTOR;
	if (fileSize <= FILE_MAX_SIZE) {
		return REPOSITION_BASED_ON_FILE_SIZE;
	} else {
		return REPOSITION_NOT_ALLOWED;
	}
}

static off_t deviceGetSize(struct VirtualFileSystemNode* virtualFileSystemNode) {
	struct ATADeviceVirtualFileSystemNode* ataDeviceVirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct ATADevice* ataDevice = ataDeviceVirtualFileSystemNode->ataDevice;
	struct ATAIdentifyReturn* ataIdentifyReturn = &ataDevice->ataIdentifyReturn;
	return ataIdentifyReturn->maxLBAAddresss28 * BYTES_PER_SECTOR;
}

static enum OpenFileDescriptionOffsetRepositionPolicy devicePartitiongetOpenFileDescriptionOffsetRepositionPolicy(struct VirtualFileSystemNode* virtualFileSystemNode) {
	struct ATADevicePartitionVirtualFileSystemNode* ataDevicePartitionVirtualFileSystemNode = (void*) virtualFileSystemNode;
	int64_t fileSize = ((int64_t) ataDevicePartitionVirtualFileSystemNode->numberOfSectors) * BYTES_PER_SECTOR;
	if (fileSize <= FILE_MAX_SIZE) {
		return REPOSITION_BASED_ON_FILE_SIZE;
	} else {
		return REPOSITION_NOT_ALLOWED;
	}
}

static off_t devicePartitionGetSize(struct VirtualFileSystemNode* virtualFileSystemNode) {
	struct ATADevicePartitionVirtualFileSystemNode* ataDevicePartitionVirtualFileSystemNode = (void*) virtualFileSystemNode;
	return ataDevicePartitionVirtualFileSystemNode->numberOfSectors * BYTES_PER_SECTOR;
}

static bool readBlocks(struct BlockDevice* blockDevice, uint64_t firstBlockId, size_t blockCount, void* buffer) {
	struct ATADevicePartitionVirtualFileSystemNode* ataDevicePartitionVirtualFileSystemNode = (void*) (((uint32_t) blockDevice) - offsetof(struct ATADevicePartitionVirtualFileSystemNode, blockDevice));

	assert(firstBlockId < ataDevicePartitionVirtualFileSystemNode->numberOfSectors);
	uint64_t sectorId = ataDevicePartitionVirtualFileSystemNode->firstSectorId + firstBlockId;

	return readSectors(ataDevicePartitionVirtualFileSystemNode->ataDevice, sectorId, buffer, blockCount) == ATA_WAIT_SUCCESS;
}

static bool writeBlocks(struct BlockDevice* blockDevice, uint64_t firstBlockId, size_t blockCount, void* buffer) {
	struct ATADevicePartitionVirtualFileSystemNode* ataDevicePartitionVirtualFileSystemNode = (void*) (((uint32_t) blockDevice) - offsetof(struct ATADevicePartitionVirtualFileSystemNode, blockDevice));

	assert(firstBlockId < ataDevicePartitionVirtualFileSystemNode->numberOfSectors);
	uint64_t sectorId = ataDevicePartitionVirtualFileSystemNode->firstSectorId + firstBlockId;

	return writeSectors(ataDevicePartitionVirtualFileSystemNode->ataDevice, sectorId, buffer, blockCount) == ATA_WAIT_SUCCESS;
}

void ataInitialize(void) {
	logDebug("Initializing ATA devices:");

	memset(&deviceOperations, 0, sizeof(struct VirtualFileSystemOperations));
	deviceOperations.open = &deviceOpen;
	deviceOperations.status = &deviceStatus;
	deviceOperations.read = &deviceRead;
	deviceOperations.write = &deviceWrite;
	deviceOperations.getOpenFileDescriptionOffsetRepositionPolicy = &devicegetOpenFileDescriptionOffsetRepositionPolicy;
	deviceOperations.getMode = &deviceGetMode;
	deviceOperations.getSize = &deviceGetSize;

	memset(&devicePartitionOperations, 0, sizeof(struct VirtualFileSystemOperations));
	devicePartitionOperations.open = &devicePartitionOpen;
	devicePartitionOperations.status = &devicePartitionStatus;
	devicePartitionOperations.read = &devicePartitionRead;
	devicePartitionOperations.getOpenFileDescriptionOffsetRepositionPolicy = &devicePartitiongetOpenFileDescriptionOffsetRepositionPolicy;
	devicePartitionOperations.getMode = &devicePartitionGetMode;
	devicePartitionOperations.getSize = &devicePartitionGetSize;

	primaryIDEChannel.id = 0;
	primaryIDEChannel.commandsRegistersBase = 0x1F0;
	primaryIDEChannel.controlRegistersBase = 0x3F6;
	initializeDevices(&primaryIDEChannel);

	secondaryIDEChannel.id = 1;
	secondaryIDEChannel.commandsRegistersBase = 0x170;
	secondaryIDEChannel.controlRegistersBase = 0x376;
	initializeDevices(&secondaryIDEChannel);

	struct ATADevice* ataDevices[NUMBER_OF_ATA_DEVICES];
	ataDevices[0] = &primaryIDEChannel.masterATADevice;
	ataDevices[1] = &primaryIDEChannel.slaveATADevice;
	ataDevices[2] = &secondaryIDEChannel.masterATADevice;
	ataDevices[3] = &secondaryIDEChannel.slaveATADevice;

	for (int i = 0; i < NUMBER_OF_ATA_DEVICES; i++) {
		struct ATADevice* ataDevice = ataDevices[i];

		if (ataDevice->operational) {
			struct ATAIdentifyReturn* ataIdentifyReturn = &ataDevice->ataIdentifyReturn;

			const int deviceFileNameBufferSize = 16;
			char deviceFileNameBuffer[deviceFileNameBufferSize];
			sprintf(deviceFileNameBuffer, "hd%c", 'a' + i);

			bool boundToFile = !canUse48BitAddressFeature(ataIdentifyReturn) ||
					ataIdentifyReturn->maxLBAAddresss48 == (uint64_t) ataIdentifyReturn->maxLBAAddresss28;
			if (boundToFile) {
				/* Register a device file associated with the ATA device. */
				struct ATADeviceVirtualFileSystemNode* ataDeviceVirtualFileSystemNode = &ataDevicesVirtualFileSystemNodes[i];
				memset(ataDeviceVirtualFileSystemNode, 0, sizeof(struct ATADeviceVirtualFileSystemNode));
				ataDeviceVirtualFileSystemNode->ataDevice = ataDevice;
				ataDeviceVirtualFileSystemNode->mode = S_IFBLK | S_IRUSR | S_IWUSR;

				struct VirtualFileSystemNode* virtualFileSystemNode = &ataDeviceVirtualFileSystemNode->virtualFileSystemNode;
				virtualFileSystemNode->operations = &deviceOperations;

				devicesFileSystemRegisterDevice(virtualFileSystemNode, deviceFileNameBuffer);
			}

			int stringStreamWriterBufferSize = 1024;
			char stringStreamWriterBuffer[stringStreamWriterBufferSize];
			struct StringStreamWriter stringStreamWriter;
			stringStreamWriterInitialize(&stringStreamWriter, stringStreamWriterBuffer, stringStreamWriterBufferSize);

			if (boundToFile) {
				streamWriterFormat(&stringStreamWriter.streamWriter, "ATA device (type \"%s\") bound to file \"%s\":\n",
						ATA_DEVICE_TYPE_NAMES[ataDevice->deviceType], deviceFileNameBuffer);
			} else {
				streamWriterFormat(&stringStreamWriter.streamWriter, "ATA device (type \"%s\") could not be bound to any file because it requires LBA 48:\n",
						ATA_DEVICE_TYPE_NAMES[ataDevice->deviceType], deviceFileNameBuffer);
			}
			streamWriterFormat(&stringStreamWriter.streamWriter, "  model=\"%s\"\n", ataDevice->model);
			streamWriterFormat(&stringStreamWriter.streamWriter, "  firmwareRevision=\"%.8s\"\n", ataIdentifyReturn->firmwareRevision);
			streamWriterFormat(&stringStreamWriter.streamWriter, "  serial=\"%.20s\"\n", ataIdentifyReturn->serial);
			streamWriterFormat(&stringStreamWriter.streamWriter, "  sectors (48)=%llX\n", ataIdentifyReturn->maxLBAAddresss48);
			streamWriterFormat(&stringStreamWriter.streamWriter, "  sectors (28)=%d\n", ataIdentifyReturn->maxLBAAddresss28);

			if (ataDevice->partitionCount > 0) {
				streamWriterFormat(&stringStreamWriter.streamWriter, "  partitions:\n");
				for (int j = 0; j < ataDevice->partitionCount; j++) {
					streamWriterFormat(&stringStreamWriter.streamWriter, "    [%d]:\n", j);

					struct ATADevicePartition* ataDevicePartition = &ataDevice->partitions[j];
					struct MBRPartitionTableEntry* partitionTableEntry = ataDevicePartition->partitionTableEntry;

					streamWriterFormat(&stringStreamWriter.streamWriter, "      partitionType: %0.2X\n", partitionTableEntry->partitionType);
					streamWriterFormat(&stringStreamWriter.streamWriter, "      driveAttributes: %0.2X\n", partitionTableEntry->driveAttributes);
					streamWriterFormat(&stringStreamWriter.streamWriter, "      lbaAddressPartitionFirstSector: %d\n", partitionTableEntry->lbaAddressPartitionFirstSector);
					streamWriterFormat(&stringStreamWriter.streamWriter, "      numberOfSectors: %d\n", partitionTableEntry->numberOfSectors);

					assert(partitionTableEntry->partitionType == MBR_PARTITION_TYPE_NATIVE_LINUX_FILE_SYSTEM);

					uint64_t devicePartitionLastSectorId = (uint64_t) partitionTableEntry->lbaAddressPartitionFirstSector + (uint64_t) (partitionTableEntry->numberOfSectors - 1);
					/* Will LBA 28 be able to read/write this partition? */
					if (devicePartitionLastSectorId <= 0x0FFFFFFF) {
						/* Register a device file associated with the partition. */
						const int devicePartitionFileNameBufferSize = 16;
						char devicePartitionFileNameBuffer[devicePartitionFileNameBufferSize];

						int devicePartitionId = i * MBR_PARTITION_TABLE_LENGTH + j;
						struct ATADevicePartitionVirtualFileSystemNode* ataDevicePartitionVirtualFileSystemNode = &ataDevicesPartitionsVirtualFileSystemNodes[devicePartitionId];
						ataDevicePartitionVirtualFileSystemNode->ataDevice = ataDevice;
						ataDevicePartitionVirtualFileSystemNode->firstSectorId = partitionTableEntry->lbaAddressPartitionFirstSector;
						ataDevicePartitionVirtualFileSystemNode->numberOfSectors = partitionTableEntry->numberOfSectors;
						ataDevicePartitionVirtualFileSystemNode->id = (uint8_t) devicePartitionId;
						ataDevicePartitionVirtualFileSystemNode->mode = S_IFBLK | S_IS_MOUNTABLE_BLOCK_DEVICE | S_IRUSR | S_IWUSR;

						sprintf(devicePartitionFileNameBuffer, "%s%d", deviceFileNameBuffer, j);

						struct BlockDevice* blockDevice = &ataDevicePartitionVirtualFileSystemNode->blockDevice;
						blockDevice->id = myosCalculateUniqueId(ATA_DEVICE_PARTITION_ID, ataDevicePartitionVirtualFileSystemNode->id);
						blockDevice->maximumBlocksPerRead = 256;
						blockDevice->blockSize = BYTES_PER_SECTOR;
						blockDevice->blockCount = partitionTableEntry->numberOfSectors;
						blockDevice->readBlocks = &readBlocks;
						blockDevice->writeBlocks = &writeBlocks;
						ataDevicePartitionVirtualFileSystemNode->blockDeviceVirtualFileSystemNode.blockDevice = blockDevice;

						struct VirtualFileSystemNode* virtualFileSystemNode = &ataDevicePartitionVirtualFileSystemNode->blockDeviceVirtualFileSystemNode.virtualFileSystemNode;
						virtualFileSystemNode->operations = &devicePartitionOperations;
						devicesFileSystemRegisterDevice(virtualFileSystemNode, devicePartitionFileNameBuffer);

						streamWriterFormat(&stringStreamWriter.streamWriter, "      bound to file \"%s\"\n", devicePartitionFileNameBuffer);

					} else {
						streamWriterFormat(&stringStreamWriter.streamWriter, "      could not be bound to any file because it requires LBA 48\n");
					}
				}

			} else {
				streamWriterFormat(&stringStreamWriter.streamWriter, "    no recognizable partitions found\n");
			}

			stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
			logDebug("%s", stringStreamWriterBuffer);
		}
	}

	logDebug("After ATA devices initialization");
}
