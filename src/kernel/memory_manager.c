/*
 * Copyright 2022 Luis Henrique O. Rios
 *
 * This file is part of MyOS kernel.
 *
 * MyOS kernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MyOS kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MyOS. If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>

#include "kernel/log.h"
#include "kernel/memory_manager.h"
#include "kernel/x86.h"

#include "util/math_utils.h"
#include "util/string_stream_writer.h"

#define FIRST_AVAILABLE_PAGE_FRAME_ADDRESS 0x100000

static uint32_t* systemPageTables;
static uint32_t systemPageTablesCount;

extern uint32_t FIRST_PAGE_FRAME_ADDRESS;

static struct DoubleLinkedListElement* pageFrameListElements;
static uint32_t firstFrameAddress;
static uint32_t firstInvalidPageFrameAddress;
static uint32_t firstUserSpacePageFrameIndex;
static int availablePageFramesCount;
static struct DoubleLinkedList kernelSpaceAvailablePageFrameList;
static struct DoubleLinkedList userSpaceAvailablePageFrameList;

#define RESERVATION_ENTRIES_ARRAY_LENGTH 64
static int reservedPageFrameCount = 0;
static int availableReservationEntries[RESERVATION_ENTRIES_ARRAY_LENGTH];
static int totalReservationEntries[RESERVATION_ENTRIES_ARRAY_LENGTH];
static int reservationEntryCount = 0;

/*
 * Memory layout:
 *
 * Physical memory:
 * - From 0x00000000 to 0x00100000 (first 1 Megabyte): only accessed from kernel code. It is only used to perform I/O operations.
 * - From 0x00100000 to (variable): only accessed from kernel code. It contains kernel code and static data.
 * - From (variable) to memory end: accessed from kernel code and user code. It contains all page frames available.
 *
 * Virtual memory (each process):
 * - From 0x00000000 to 0x40000000 (first 1 Gigabyte): only accessed (using this address range) from kernel code.
 * 	It contains the kernel code segment and the kernel data segment.
 * 	Each address maps to the same physical address.
 * 	Some page frames may also be mapped to different addresses on user process virtual address range.
 * - From 0x40000000 to (variable): user process code segment.
 * - From (variable) to (variable): user process data segment.
 * - From (variable) to 0xFFFFFFFF: user process stack segment.
 */

int memoryManagerReserveMemoryOnKernelSpace(uint32_t pageFrameCount) {
	if (doubleLinkedListSize(&kernelSpaceAvailablePageFrameList) >= reservedPageFrameCount + pageFrameCount
			&& reservationEntryCount + 1 < RESERVATION_ENTRIES_ARRAY_LENGTH) {
		availableReservationEntries[reservationEntryCount] = pageFrameCount;
		totalReservationEntries[reservationEntryCount] = pageFrameCount;
		reservedPageFrameCount += pageFrameCount;
		return reservationEntryCount++;
	} else {
		return -1;
	}
}

uint32_t memoryManagerGetUserSpaceAvailablePageFrameCount(void) {
	return doubleLinkedListSize(&userSpaceAvailablePageFrameList);
}

uint32_t memoryManagerGetKernelSpaceAvailablePageFrameCount(void) {
	return doubleLinkedListSize(&kernelSpaceAvailablePageFrameList);
}

static uint32_t calculatePageFrameListElementIndex(struct DoubleLinkedListElement* pageFrameListElement) {
	uint32_t index = ((uint32_t) pageFrameListElement - (uint32_t) pageFrameListElements)
		/ sizeof(struct DoubleLinkedListElement);
	assert(index < availablePageFramesCount);
	return index;
}

void memoryManagerInitialize(uint32_t multibootAmountOfUpperMemory) {
	/* Calculate the memory limits. */
	firstInvalidPageFrameAddress = FIRST_AVAILABLE_PAGE_FRAME_ADDRESS + multibootAmountOfUpperMemory * 1024;
	assert(firstInvalidPageFrameAddress % PAGE_FRAME_SIZE == 0);
	if ((uint32_t) &FIRST_PAGE_FRAME_ADDRESS >= firstInvalidPageFrameAddress) {
		logDebug("There is no enough memory to proceed");
		x86Ring0Stop();
	}

	firstFrameAddress = (uint32_t) &FIRST_PAGE_FRAME_ADDRESS;
	assert(firstFrameAddress % PAGE_FRAME_SIZE == 0);

	/* Request some page frames that will be used to store page table entries. It needs to be able to map all available memory. */
	systemPageTablesCount = mathUtilsCeilOfUint32Division(firstInvalidPageFrameAddress, PAGE_TABLE_LENGTH * PAGE_FRAME_SIZE);
	systemPageTables = (uint32_t*) firstFrameAddress;
	firstFrameAddress += systemPageTablesCount * PAGE_FRAME_SIZE;
	if (firstFrameAddress >= firstInvalidPageFrameAddress) {
		logDebug("There is no enough memory to proceed");
		x86Ring0Stop();
	}

	/* It initializes the mapping that handles the kernel space. */
	for (uint32_t i = 0; i < systemPageTablesCount * PAGE_TABLE_LENGTH; i++) {
		uint32_t systemPageTableEntry = (i << 12) | PAGE_ENTRY_PRESENT | PAGE_ENTRY_READ_WRITE | PAGE_ENTRY_SYSTEM | PAGE_ENTRY_SIZE_4_KBYTES;

		if (i < FIRST_AVAILABLE_PAGE_FRAME_ADDRESS / PAGE_FRAME_SIZE) {
			systemPageTableEntry = systemPageTableEntry | PAGE_ENTRY_WRITE_THROUGH | PAGE_ENTRY_CACHE_DISABLED;
		}

		if (i < SYSTEM_PAGE_TABLES_COUNT * PAGE_TABLE_LENGTH) {
			systemPageTableEntry = systemPageTableEntry | PAGE_ENTRY_GLOBAL;
		} else {
			systemPageTableEntry = systemPageTableEntry | PAGE_ENTRY_LOCAL;
		}
		systemPageTables[i] = systemPageTableEntry;
	}

	/* Request some page frames that will be used to store the list elements. */
	availablePageFramesCount = (firstInvalidPageFrameAddress - firstFrameAddress) / PAGE_FRAME_SIZE;
	uint32_t pageFramesToStoreListElements = mathUtilsCeilOfUint32Division(availablePageFramesCount * sizeof(struct DoubleLinkedListElement), PAGE_FRAME_SIZE);
	availablePageFramesCount -= pageFramesToStoreListElements;
	pageFrameListElements = (struct DoubleLinkedListElement*) firstFrameAddress;
	firstFrameAddress += pageFramesToStoreListElements * PAGE_FRAME_SIZE;
	if (firstFrameAddress >= firstInvalidPageFrameAddress) {
		logDebug("There is no enough memory to proceed");
		x86Ring0Stop();
	}

	/* Calculate the limits related with the memory available to be allocated. */
	uint32_t firstUserSpacePageFramePhysicalAddress = (SYSTEM_PAGE_TABLES_COUNT * PAGE_TABLE_LENGTH * PAGE_FRAME_SIZE);
	if (firstFrameAddress >= firstUserSpacePageFramePhysicalAddress) {
		logDebug("There is no enough memory to proceed");
		x86Ring0Stop();
	}
	firstUserSpacePageFrameIndex = (firstUserSpacePageFramePhysicalAddress - firstFrameAddress) / PAGE_FRAME_SIZE;

	assert(firstUserSpacePageFrameIndex >= availablePageFramesCount
		|| firstUserSpacePageFramePhysicalAddress == (uint32_t) memoryManagerGetPageFramePhysicalAddress(&pageFrameListElements[firstUserSpacePageFrameIndex]));
	doubleLinkedListInitialize(&kernelSpaceAvailablePageFrameList);
	doubleLinkedListInitialize(&userSpaceAvailablePageFrameList);
	for (int i = availablePageFramesCount - 1; i >= 0; i--) {
		if (i >= firstUserSpacePageFrameIndex) {
			doubleLinkedListInsertAfterLast(&userSpaceAvailablePageFrameList, &pageFrameListElements[i]);
			assert(firstUserSpacePageFramePhysicalAddress <= (uint32_t) memoryManagerGetPageFramePhysicalAddress(&pageFrameListElements[i]));
		} else {
			doubleLinkedListInsertAfterLast(&kernelSpaceAvailablePageFrameList, &pageFrameListElements[i]);
			assert(firstUserSpacePageFramePhysicalAddress > (uint32_t) memoryManagerGetPageFramePhysicalAddress(&pageFrameListElements[i]));
		}
	}
	assert(availablePageFramesCount == doubleLinkedListSize(&kernelSpaceAvailablePageFrameList)
		+ doubleLinkedListSize(&userSpaceAvailablePageFrameList));
	assert(firstFrameAddress == (uint32_t) memoryManagerGetPageFramePhysicalAddress(&pageFrameListElements[0]));
	assert(firstInvalidPageFrameAddress - PAGE_FRAME_SIZE == (uint32_t) memoryManagerGetPageFramePhysicalAddress(&pageFrameListElements[availablePageFramesCount - 1]));

	logDebug("Memory manager: \n"
		"  Limits information:\n"
		"    FIRST_PAGE_FRAME_ADDRESS=%p\n"
		"    firstInvalidPageFrameAddress=%p\n"

		"  Static allocation information:\n"
		"    systemPageTablesCount=%d\n"

		"    pageFramesToStoreListElements=%d\n"
		"  Dynamic allocation information:\n"
		"    firstFrameAddress=%p\n"
		"    firstUserSpacePageFrameIndex=%d\n"
		"    firstUserSpacePageFramePhysicalAddress=%p\n"
		"    availablePageFramesCount=%d\n"
		"    kernelSpaceAvailablePageFrames size=%d\n"
		"    userSpaceAvailablePageFrames size=%d\n",

		&FIRST_PAGE_FRAME_ADDRESS,
		firstInvalidPageFrameAddress,

		systemPageTablesCount,
		pageFramesToStoreListElements,

		firstFrameAddress,
		firstUserSpacePageFrameIndex,
		firstUserSpacePageFramePhysicalAddress,
		availablePageFramesCount,
		doubleLinkedListSize(&kernelSpaceAvailablePageFrameList),
		doubleLinkedListSize(&userSpaceAvailablePageFrameList));
}

uint32_t memoryManagerGetSystemPageTablesCount(void) {
	return systemPageTablesCount;
}

void* memoryManagerGetSystemPageTableAddress(uint32_t pageTableIndex) {
	return &systemPageTables[pageTableIndex * PAGE_TABLE_LENGTH];
}

void memoryManagerRemovePageTableMapping(uint32_t* pageDirectory, uint32_t physicalAddress) {
	assert(physicalAddress % PAGE_FRAME_SIZE == 0);

	for (int i = 0; i < PAGE_DIRECTORY_LENGTH; i++) {
		if ((pageDirectory[i] & 0xFFFFF000) == physicalAddress) {
			pageDirectory[i] = 0;
			break;
		}
	}
}

void memoryManagerRemovePageMapping(uint32_t* pageDirectory, uint32_t virtualAddress, uint32_t physicalAddress) {
	assert(virtualAddress % PAGE_FRAME_SIZE == 0);
	assert(physicalAddress % PAGE_FRAME_SIZE == 0);

	uint32_t pageDirectoryIndex = (virtualAddress >> 22);
	uint32_t pageTableIndex = (virtualAddress >> 12) & 0x3FF;

	assert(pageDirectory[pageDirectoryIndex] & PAGE_ENTRY_PRESENT);
	assert(pageDirectory[pageDirectoryIndex] & PAGE_ENTRY_USER);
	assert((pageDirectory[pageDirectoryIndex] & PAGE_ENTRY_GLOBAL) == 0);
	uint32_t* pageTable = (uint32_t*) (pageDirectory[pageDirectoryIndex] & 0xFFFFF000);

	assert(pageTable[pageTableIndex] & PAGE_ENTRY_USER);
	assert((pageTable[pageTableIndex] & PAGE_ENTRY_GLOBAL) == 0);
	assert((pageTable[pageTableIndex] & 0xFFFFF000) ==  physicalAddress);
	pageTable[pageTableIndex] = PAGE_ENTRY_NOT_PRESENT;
}

bool memoryManagerConfigureMapping(struct DoubleLinkedListElement** newPageFrame, uint32_t* pageDirectory, uint32_t virtualAddress,
		uint32_t physicalAddress, uint32_t flags) {
	assert((flags & PAGE_ENTRY_SIZE_4_MBYTES) == 0);
	assert(((uint32_t) pageDirectory) % PAGE_FRAME_SIZE == 0);
	assert(virtualAddress % PAGE_FRAME_SIZE == 0);
	assert(physicalAddress % PAGE_FRAME_SIZE == 0);
	assert(!(flags & ~0xFFF));

	*newPageFrame = NULL;

	uint32_t pageDirectoryIndex = (virtualAddress >> 22);
	assert(pageDirectoryIndex < PAGE_DIRECTORY_LENGTH);
	uint32_t pageDirectoryEntry = pageDirectory[pageDirectoryIndex];

	uint32_t* pageTable;
	if (!(pageDirectoryEntry & PAGE_ENTRY_PRESENT)) {
		*newPageFrame = memoryManagerAcquirePageFrame(true, -1);
		if (*newPageFrame == NULL) {
			return false;
		}
		pageTable = (uint32_t*) memoryManagerGetPageFramePhysicalAddress(*newPageFrame);
		assert((uint32_t) pageTable % PAGE_FRAME_SIZE == 0);

		for (int i = 0; i < PAGE_TABLE_LENGTH; i++) {
			pageTable[i] = PAGE_ENTRY_NOT_PRESENT;
		}

		pageDirectoryEntry = (uint32_t) pageTable | flags;
		pageDirectory[pageDirectoryIndex] = pageDirectoryEntry;

	} else {
		pageTable = (uint32_t*) (pageDirectoryEntry & 0xFFFFF000);
	}

	uint32_t pageTableIndex = (virtualAddress >> 12) & 0x3FF;
	assert(pageTableIndex < PAGE_TABLE_LENGTH);
	uint32_t pageTableEntry = physicalAddress | flags;
	assert(pageTable[pageTableIndex] == PAGE_ENTRY_NOT_PRESENT);
	pageTable[pageTableIndex] = pageTableEntry;

	return true;
}

struct DoubleLinkedListElement* memoryManagerAcquirePageFrame(bool kernelSpace, int reservationId) {
	assert(reservationId < 0 || kernelSpace);

	struct DoubleLinkedList* list = NULL;
	if (kernelSpace) {
		if (doubleLinkedListSize(&kernelSpaceAvailablePageFrameList) > 0 && (reservationId < 0 || availableReservationEntries[reservationId] > 0)) {
			list = &kernelSpaceAvailablePageFrameList;
			if (reservationId >= 0) {
				availableReservationEntries[reservationId]--;
				reservedPageFrameCount--;
			}
		}

	} else {
		if (doubleLinkedListSize(&userSpaceAvailablePageFrameList) > 0) {
			list = &userSpaceAvailablePageFrameList;

		} else {
			return memoryManagerAcquirePageFrame(true, -1);
		}
	}

	if (list != NULL) {
		struct DoubleLinkedListElement* pageFrameListElement = doubleLinkedListRemoveFirst(list);
		assert((calculatePageFrameListElementIndex(pageFrameListElement) >= firstUserSpacePageFrameIndex) ^ kernelSpace);
		return pageFrameListElement;

	} else {
		return NULL;
	}
}

void memoryManagerReleasePageFrame(struct DoubleLinkedListElement* pageFrameListElement, int reservationId) {
	uint32_t index = calculatePageFrameListElementIndex(pageFrameListElement);

	struct DoubleLinkedList* list = NULL;
	if (index >= firstUserSpacePageFrameIndex) {
		assert(reservationId == -1);
		list = &userSpaceAvailablePageFrameList;
	} else {
		list = &kernelSpaceAvailablePageFrameList;
		if (reservationId >= 0) {
			assert(availableReservationEntries[reservationId] <= totalReservationEntries[reservationId]);
			if (availableReservationEntries[reservationId] < totalReservationEntries[reservationId]) {
				availableReservationEntries[reservationId]++;
				reservedPageFrameCount++;
			}
		}
	}

	doubleLinkedListInsertBeforeFirst(list, pageFrameListElement);
}

uint32_t memoryManagerGetPageFramePhysicalAddress(struct DoubleLinkedListElement* pageFrameListElement) {
	assert((uint32_t) pageFrameListElements <= (uint32_t) pageFrameListElement);

	uint32_t index = ((uint32_t) pageFrameListElement - (uint32_t) pageFrameListElements)
		/ sizeof(struct DoubleLinkedListElement);
	assert(index < availablePageFramesCount);
	return firstFrameAddress + index * PAGE_FRAME_SIZE;
}

struct DoubleLinkedListElement* memoryManagerGetPageFrameDoubleLinkedListElement(uint32_t physicalAddress) {
		assert(physicalAddress < firstInvalidPageFrameAddress);

		uint32_t index = (physicalAddress - firstFrameAddress) / PAGE_FRAME_SIZE;
		return pageFrameListElements + index;
}

APIStatusCode memoryManagerPrintDebugReport(void) {
	int bufferSize = 512;
	char buffer[bufferSize];
	struct StringStreamWriter stringStreamWriter;

	stringStreamWriterInitialize(&stringStreamWriter, buffer, bufferSize);
	streamWriterFormat(&stringStreamWriter.streamWriter, "Memory manager report:\n");
	streamWriterFormat(&stringStreamWriter.streamWriter, "  userSpaceAvailablePageFrameCount=%d\n", memoryManagerGetUserSpaceAvailablePageFrameCount());
	streamWriterFormat(&stringStreamWriter.streamWriter, "  kernelSpaceAvailablePageFrameCount=%d\n", memoryManagerGetKernelSpaceAvailablePageFrameCount());
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);

	logDebug("%s", buffer);

	return SUCCESS;
}
