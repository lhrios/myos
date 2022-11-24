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

#ifndef KERNEL_MBR_H
	#define KERNEL_MBR_H

	#include <stdint.h>

	/*
	 * References:
	 * - https://en.wikipedia.org/wiki/Partition_type
	 * - https://en.wikipedia.org/wiki/Master_boot_record
	 * - https://wiki.osdev.org/Partition_Table
	 */

	struct MBRPartitionTableEntry {
		uint8_t driveAttributes;
		uint8_t chsAddressPartitionFirstSector[3];
		uint8_t partitionType;
		uint8_t chsAddressPartitionLastSector[3];
		uint32_t lbaAddressPartitionFirstSector;
		uint32_t numberOfSectors;
	} __attribute__((packed));
	_Static_assert(sizeof(struct MBRPartitionTableEntry) == 16, "Expecting PartitionTableEntry with 16 bytes.");

	#define MBR_BOOTSTRAP_EXECUTABLE_SIZE 446
	#define MBR_PARTITION_TABLE_LENGTH 4
	#define MBR_SIGNATURE 0xAA55

	#define MBR_PARTITION_TYPE_NATIVE_LINUX_FILE_SYSTEM 0x83

	struct MBR {
		uint8_t bootstrapCode[MBR_BOOTSTRAP_EXECUTABLE_SIZE];
		struct MBRPartitionTableEntry partitionTable[MBR_PARTITION_TABLE_LENGTH];
		uint16_t signature;
	} __attribute__((packed));
	_Static_assert(sizeof(struct MBR) == 512, "Expecting MBR with 512 bytes.");

	inline __attribute__((always_inline)) bool mbrIsPartitionValid(struct MBRPartitionTableEntry* partitionTableEntry) {
		return (partitionTableEntry->driveAttributes & 0x7F) == 0;
	}

	inline __attribute__((always_inline)) bool mbrIsPartitionTypeRecognizable(struct MBRPartitionTableEntry* partitionTableEntry) {
		return partitionTableEntry->partitionType == MBR_PARTITION_TYPE_NATIVE_LINUX_FILE_SYSTEM;
	}

#endif
