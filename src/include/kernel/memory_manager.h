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

#ifndef KERNEL_MEMORY_MANAGER_H
	#define KERNEL_MEMORY_MANAGER_H

	#include <stdbool.h>
	#include <stdint.h>

	#include "kernel/memory_manager_constants.h"

	#include "kernel/process/process.h"

	#include "util/double_linked_list.h"

	void memoryManagerInitialize(uint32_t multibootAmountOfUpperMemory);
	struct DoubleLinkedListElement* memoryManagerGetPageFrameDoubleLinkedListElement(uint32_t physicalAddress);
	uint32_t memoryManagerGetPageFramePhysicalAddress(struct DoubleLinkedListElement* pageFrameListElement);
	bool memoryManagerConfigureMapping(struct DoubleLinkedListElement** newPageFrame, uint32_t* pageDirectory,
		uint32_t virtualAddress, uint32_t physicalAddress, uint32_t flags);
	struct DoubleLinkedListElement* memoryManagerAcquirePageFrame(bool kernelSpace, int reservationId);
	void memoryManagerReleasePageFrame(struct DoubleLinkedListElement* pageFrameListElement, int reservationId);
	void* memoryManagerGetSystemPageTableAddress(uint32_t pageTableIndex);
	int memoryManagerReserveMemoryOnKernelSpace(uint32_t pageFrameCount);

	void memoryManagerRemovePageTableMapping(uint32_t* pageDirectory, uint32_t physicalAddress);
	void memoryManagerRemovePageMapping(uint32_t* pageDirectory, uint32_t virtualAddress, uint32_t physicalAddress);

	uint32_t memoryManagerGetSystemPageTablesCount(void);

	uint32_t memoryManagerGetKernelSpaceAvailablePageFrameCount(void);
	uint32_t memoryManagerGetUserSpaceAvailablePageFrameCount(void);

	APIStatusCode memoryManagerPrintDebugReport(void);

	struct Process;
	inline __attribute__((always_inline)) uint32_t memoryManagerCalculateProcessFirstInvalidDataSegmentAddress(struct Process* process) {
		return DATA_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS + doubleLinkedListSize(&process->dataSegmentPageFramesList) * PAGE_FRAME_SIZE;
	}
#endif
