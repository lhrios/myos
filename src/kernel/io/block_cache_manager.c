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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "kernel/cmos.h"
#include "kernel/error_handler.h"
#include "kernel/log.h"
#include "kernel/memory_manager.h"

#include "kernel/io/block_cache_manager.h"

#include "util/b_tree.h"
#include "util/debug_utils.h"
#include "util/double_linked_list.h"
#include "util/math_utils.h"
#include "util/string_stream_writer.h"

struct CachedBlock {
	struct DoubleLinkedListElement listElement;
	struct DoubleLinkedListElement dirtyListElement;
	struct BlockDevice* blockDevice;
	uint64_t blockId;
	void* data;
	uint16_t usageCount;
	bool isDirty;
};

static struct BTree cachedBlockByBlockId;

static struct DoubleLinkedList availablePositionsList;
static struct DoubleLinkedList usedPositionsList; /* It is sorted by last access. */
static struct DoubleLinkedList dirtyList;

static void* memoryAllocatorAcquire(void *unused, size_t size) {
	struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
	if (doubleLinkedListElement != NULL) {
		return (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);

	} else {
		return NULL;
	}
}

static void memoryAllocatorRelease(void* unused, void* pointer) {
	struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerGetPageFrameDoubleLinkedListElement((uint32_t) pointer);
	memoryManagerReleasePageFrame(doubleLinkedListElement, -1);
}

static int compare(struct CachedBlock** cachedBlock1, struct CachedBlock** cachedBlock2) {
	dev_t deviceId1 = (*cachedBlock1)->blockDevice->id;
	dev_t deviceId2 = (*cachedBlock2)->blockDevice->id;
	uint64_t blockId1 = (*cachedBlock1)->blockId;
	uint64_t blockId2 = (*cachedBlock2)->blockId;

	if (deviceId1 == deviceId2 && blockId1 == blockId2) {
		return 0;

	} else if (deviceId1 == deviceId2) {
		if (blockId1 < blockId2) {
			return -1;
		} else {
			return 1;
		}

	} else {
		if (deviceId1 < deviceId2) {
			return -1;
		} else {
			return 1;
		}
	}
}

static uint64_t calculateAdjustBlockIdAndOffset(struct BlockDevice* blockDevice, uint64_t blockId, uint32_t* offset) {
	uint64_t adjustedBlockId;
	/* Are there multiple blocks per page frame? */
	if (blockDevice->blockSize < PAGE_FRAME_SIZE) {
		uint32_t deviceBlocksPerPageFrame = PAGE_FRAME_SIZE / blockDevice->blockSize;
		assert(mathUtilsIsPowerOf(deviceBlocksPerPageFrame, 2));
		uint32_t log2 = mathUtilsLog2ForPowerOf2(deviceBlocksPerPageFrame);

		/* As we are using x86 we can not perform some operations on 64 bit integers. */
		uint64_t mask = (uint64_t) ((1 << log2) - 1);
		*offset = ((uint32_t) (blockId & mask)) * blockDevice->blockSize;
		assert(*offset < PAGE_FRAME_SIZE);

		adjustedBlockId = blockId - (blockId & mask);

	} else {
		assert(blockDevice->blockSize == PAGE_FRAME_SIZE);
		adjustedBlockId = blockId;
		*offset = 0;
	}

	return adjustedBlockId;
}

void blockCacheManageReleaseReservation(struct BlockDevice* blockDevice, uint64_t blockId, bool modified) {
	uint32_t offset;
	uint64_t adjustedBlockId = calculateAdjustBlockIdAndOffset(blockDevice, blockId, &offset);

	struct CachedBlock cachedBlock;
	cachedBlock.blockDevice = blockDevice;
	cachedBlock.blockId = adjustedBlockId;

	struct CachedBlock* selectedCacheBlock = &cachedBlock;

	enum OperationResult operationResult = bTreeSearch(&cachedBlockByBlockId, &selectedCacheBlock);
	if (operationResult == B_TREE_SUCCESS) {
		assert(selectedCacheBlock->blockDevice == blockDevice);
		assert(selectedCacheBlock->blockId == adjustedBlockId);
		assert(selectedCacheBlock->usageCount > 0);

		if (!selectedCacheBlock->isDirty && modified) {
			doubleLinkedListInsertAfterLast(&dirtyList, &selectedCacheBlock->dirtyListElement);
		}
		selectedCacheBlock->isDirty = selectedCacheBlock->isDirty || modified;
		assert(!selectedCacheBlock->isDirty || doubleLinkedListContainsFoward(&dirtyList, &selectedCacheBlock->dirtyListElement));
		if (--selectedCacheBlock->usageCount == 0) {
			assert(doubleLinkedListContainsBackward(&usedPositionsList, &selectedCacheBlock->listElement));
			doubleLinkedListRemove(&usedPositionsList, &selectedCacheBlock->listElement);

			doubleLinkedListInsertAfterLast(&availablePositionsList, &selectedCacheBlock->listElement); /* LRU block goes last */
		}

	} else {
		assert(false);
	}
}

APIStatusCode blockCacheManagerReadAndReserveByOffset(struct BlockDevice* blockDevice, uint32_t offset, uint32_t blockCount, void** data, uint64_t* firstBlockId) {
	assert(offset % blockDevice->blockSize == 0);

	*firstBlockId = offset / blockDevice->blockSize;
	return blockCacheManagerReadAndReserve(blockDevice, *firstBlockId, blockCount, data);
}

static APIStatusCode commonBlockReserve(struct BlockDevice* blockDevice, uint64_t firstBlockId, uint32_t blockCount, bool read, void** data) {
	assert(blockCount * blockDevice->blockSize <= PAGE_FRAME_SIZE);

	APIStatusCode result = SUCCESS;

	uint32_t offset;
	uint64_t adjustedBlockId = calculateAdjustBlockIdAndOffset(blockDevice, firstBlockId, &offset);

	struct CachedBlock cachedBlock;
	cachedBlock.blockDevice = blockDevice;
	cachedBlock.blockId = adjustedBlockId;

	struct CachedBlock* selectedCacheBlock = &cachedBlock;

	enum OperationResult operationResult = bTreeSearch(&cachedBlockByBlockId, &selectedCacheBlock);
	if (operationResult == B_TREE_SUCCESS) {
		assert(selectedCacheBlock != &cachedBlock);

		if (selectedCacheBlock->usageCount == 0) {
			assert(doubleLinkedListContainsBackward(&availablePositionsList, &selectedCacheBlock->listElement));
			doubleLinkedListRemove(&availablePositionsList, &selectedCacheBlock->listElement);
		} else {
			assert(doubleLinkedListContainsBackward(&usedPositionsList, &selectedCacheBlock->listElement));
			doubleLinkedListRemove(&usedPositionsList, &selectedCacheBlock->listElement);
		}

		*data = selectedCacheBlock->data + offset;
		selectedCacheBlock->usageCount++;

		doubleLinkedListInsertAfterLast(&usedPositionsList, &selectedCacheBlock->listElement); /* LRU block goes last */

	} else {
		assert(operationResult == B_TREE_NOTHING_FOUND);

		selectedCacheBlock = NULL;

		/* Is there any available position? */
		if (doubleLinkedListSize(&availablePositionsList) > 0) {
			selectedCacheBlock = (void*) doubleLinkedListRemoveFirst(&availablePositionsList);
			assert(selectedCacheBlock->usageCount == 0);

			operationResult = bTreeRemove(&cachedBlockByBlockId, &selectedCacheBlock);
			if (operationResult == B_TREE_SUCCESS) {
				assert(selectedCacheBlock->data != NULL);

				/* Do we need to write data back to the disk? */
				if (selectedCacheBlock->isDirty) {
					selectedCacheBlock->isDirty = false;
					assert(doubleLinkedListContainsFoward(&dirtyList, &selectedCacheBlock->dirtyListElement));
					blockDevice->writeBlocks(blockDevice, selectedCacheBlock->blockId, PAGE_FRAME_SIZE / blockDevice->blockSize, selectedCacheBlock->data);
					doubleLinkedListRemove(&dirtyList, &selectedCacheBlock->dirtyListElement);
				}

			} else if (selectedCacheBlock->data == NULL) {
				struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
				if (doubleLinkedListElement != NULL) {
					selectedCacheBlock->data = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
				} else {
					result = ENOMEM;
				}
			}
			assert(!selectedCacheBlock->isDirty);

		} else {
			result = ENOMEM;
		}

		if (result == SUCCESS) {
			/* Read the data. */
			if (read || PAGE_FRAME_SIZE > blockCount * blockDevice->blockSize) {
				/*
				 * If it is reading less blocks than a page frame can hold we, necessarily, need to read the data.
				 * Otherwise, if later, a block that belongs to the same page frame is requested, the data will not be there and it will not be read as well (as there will be no cache miss).
				 */
				assert(blockDevice->maximumBlocksPerRead >= PAGE_FRAME_SIZE / blockDevice->blockSize);
				if (!blockDevice->readBlocks(blockDevice, adjustedBlockId, PAGE_FRAME_SIZE / blockDevice->blockSize, selectedCacheBlock->data)) {
					assert(false); /* All I/O errors are considered fatal. Therefore, this code will never be executed. */
					result = EIO;
				}
			}

			if (result == SUCCESS) {
				selectedCacheBlock->blockDevice = blockDevice;
				selectedCacheBlock->blockId = adjustedBlockId;

				operationResult = bTreeInsert(&cachedBlockByBlockId, &selectedCacheBlock);
				if (operationResult == B_TREE_SUCCESS) {
					selectedCacheBlock->usageCount = 1;
					selectedCacheBlock->isDirty = false;
					doubleLinkedListInsertAfterLast(&usedPositionsList, &selectedCacheBlock->listElement);
					*data = selectedCacheBlock->data + offset;

				} else {
					assert(operationResult == B_TREE_NOT_ENOUGH_MEMORY);
					result = ENOMEM;
				}
			}
		}

		if (result != SUCCESS && selectedCacheBlock != NULL) {
			doubleLinkedListInsertBeforeFirst(&availablePositionsList, &selectedCacheBlock->listElement);
		}
	}

	return result;
}

APIStatusCode blockCacheManagerReserve(struct BlockDevice* blockDevice, uint64_t firstBlockId, uint32_t blockCount, void** data) {
	return commonBlockReserve(blockDevice, firstBlockId, blockCount, false, data);
}

APIStatusCode blockCacheManagerReadAndReserve(struct BlockDevice* blockDevice, uint64_t firstBlockId, uint32_t blockCount, void** data) {
	return commonBlockReserve(blockDevice, firstBlockId, blockCount, true, data);
}

APIStatusCode blockCacheManagerInitialize(uint32_t maxPageFramesToCacheBlocks) {
	APIStatusCode result = SUCCESS;

	int worstCaseNodeCount = bTreeWorstCaseNodeCountToStore(PAGE_FRAME_SIZE, sizeof(void*), maxPageFramesToCacheBlocks);
	logDebug("It will reserve %d page frames in order to store cached blocks b-tree's meta data", worstCaseNodeCount);
	int reservationId = memoryManagerReserveMemoryOnKernelSpace(worstCaseNodeCount);

	if (reservationId == -1) {
		result = ENOMEM;

	} else {
		doubleLinkedListInitialize(&availablePositionsList);
		doubleLinkedListInitialize(&usedPositionsList);
		doubleLinkedListInitialize(&dirtyList);

		struct DoubleLinkedList pageFrameList;
		doubleLinkedListInitialize(&pageFrameList);

		while (doubleLinkedListSize(&availablePositionsList) < maxPageFramesToCacheBlocks) {
			struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
			if (doubleLinkedListElement == NULL) {
				result = ENOMEM;
				break;

			} else {
				doubleLinkedListInsertAfterLast(&pageFrameList, doubleLinkedListElement);
				struct CachedBlock* cachedBlocks = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
				for (uint32_t i = 0; i < PAGE_FRAME_SIZE / sizeof(struct CachedBlock) && doubleLinkedListSize(&availablePositionsList) < maxPageFramesToCacheBlocks; i++) {
					struct CachedBlock* cachedBlock = &cachedBlocks[i];
					memset(cachedBlock, 0, sizeof(struct CachedBlock));
					doubleLinkedListInsertAfterLast(&availablePositionsList, &cachedBlock->listElement);
				}
			}
		}

		if (result != SUCCESS) {
			while (doubleLinkedListSize(&pageFrameList) > 0) {
				struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListRemoveFirst(&pageFrameList);
				memoryManagerReleasePageFrame(doubleLinkedListElement, -1);
			}

		} else {
			bTreeInitialize(&cachedBlockByBlockId, PAGE_FRAME_SIZE, sizeof(void*),
				(void*) reservationId,
				(void* (*)(void*, size_t)) &memoryAllocatorAcquire,
				(void (*)(void*, void*)) &memoryAllocatorRelease,
				(int (*)(const void*, const void*)) &compare
			);
		}
	}

	return result;
}

APIStatusCode blockCacheManagerPrintDebugReport(void) {
	APIStatusCode result = SUCCESS;

	struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
	if (doubleLinkedListElement != NULL) {
		char* buffer = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
		struct StringStreamWriter stringStreamWriter;
		stringStreamWriterInitialize(&stringStreamWriter, buffer, PAGE_FRAME_SIZE);
		streamWriterFormat(&stringStreamWriter.streamWriter, "\nBlock cache manager report:\n");
		streamWriterFormat(&stringStreamWriter.streamWriter, "  cachedBlockByBlockId size: %d\n", bTreeSize(&cachedBlockByBlockId));
		streamWriterFormat(&stringStreamWriter.streamWriter, "  availablePositionsList size: %d\n", doubleLinkedListSize(&availablePositionsList));
		streamWriterFormat(&stringStreamWriter.streamWriter, "  usedPositionsList size: %d\n", doubleLinkedListSize(&usedPositionsList));
		streamWriterFormat(&stringStreamWriter.streamWriter, "  dirtyList size: %d\n", doubleLinkedListSize(&dirtyList));
		stringStreamWriterForceTerminationCharacter(&stringStreamWriter);

		logDebug("%s", buffer);
		memoryManagerReleasePageFrame(doubleLinkedListElement, -1);

	} else {
		result = ENOMEM;
	}

	return result;
}

static struct CachedBlock* getCachedBlockFromDirtyListElement(struct DoubleLinkedListElement* listElement) {
	if (listElement != NULL) {
		uint32_t address = ((uint32_t) listElement) - offsetof(struct CachedBlock, dirtyListElement);
		return (struct CachedBlock*) address;
	} else {
		return NULL;
	}
}

void blockCacheManageFlush(void) {
	int countOfBlocksToFlush = doubleLinkedListSize(&dirtyList);
	time_t before = cmosGetUnixTime();

	while (doubleLinkedListSize(&dirtyList) > 0) {
		struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListRemoveFirst(&dirtyList);
		struct CachedBlock* cachedBlock = getCachedBlockFromDirtyListElement(doubleLinkedListElement);
		struct BlockDevice* blockDevice = cachedBlock->blockDevice;
		assert(cachedBlock->isDirty);
		assert(cachedBlock->data != NULL);
		if (!cachedBlock->blockDevice->writeBlocks(cachedBlock->blockDevice, cachedBlock->blockId, PAGE_FRAME_SIZE / blockDevice->blockSize, cachedBlock->data)) {
			errorHandlerFatalError("There was a fatal error while trying to flush cached blocks: %s", sys_errlist[EIO]);
		}
		cachedBlock->isDirty = false;
	}

	time_t after = cmosGetUnixTime();
	logDebug("It took %d (%d - %d) to flush %d blocks", (after - before), after, before, countOfBlocksToFlush);

#ifndef N_DEBUG
	struct DoubleLinkedListElement* doubleLinkedListElement;

	doubleLinkedListElement = doubleLinkedListFirst(&availablePositionsList);
	while (doubleLinkedListElement != NULL) {
		struct CachedBlock* cachedBlock = (void*) doubleLinkedListElement;
		assert(!cachedBlock->isDirty);
		doubleLinkedListElement = doubleLinkedListElement->next;
	}

	doubleLinkedListElement = doubleLinkedListFirst(&usedPositionsList);
	while (doubleLinkedListElement != NULL) {
		struct CachedBlock* cachedBlock = (void*) doubleLinkedListElement;
		assert(!cachedBlock->isDirty);
		doubleLinkedListElement = doubleLinkedListElement->next;
	}
#endif

}

void blockCacheManageClear(void) {
	assert (doubleLinkedListSize(&usedPositionsList) == 0);
	bTreeClear(&cachedBlockByBlockId);
}
