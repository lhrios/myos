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

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "user/util/checksum.h"

#include "util/double_linked_list.h"
#include "util/math_utils.h"

struct MemoryChunk {
	struct DoubleLinkedListElement listElement;
	struct DoubleLinkedListElement availableListElement;
	size_t size;
	uint16_t flags;
	uint16_t checksum;
};
_Static_assert(sizeof(struct MemoryChunk) % 4 == 0, "Expecting an aligned (4 bytes) MemoryChunk.");

#define DATA_SEGMENT_MIN_INCREMENT (1 * 1024 * 1024)
#define AVAILABLE_FLAG_MASK 0x0001
#define MIN_CHUNK_SIZE sizeof(uint32_t)
_Static_assert(sizeof(struct MemoryChunk) + MIN_CHUNK_SIZE <= DATA_SEGMENT_MIN_INCREMENT, "The minimum increment is too small.");

static struct DoubleLinkedList memoryChunkList;
static struct DoubleLinkedList availableMemoryChunkList;

static void* (*sbrk)(int increment);

#ifndef NDEBUG
	static uint16_t calculateChecksum(struct MemoryChunk* memoryChunk) {
		return fletcher16(memoryChunk, sizeof(struct MemoryChunk) - sizeof(uint16_t));
	}

	static bool doesChecksumMatchs(struct MemoryChunk* memoryChunk) {
		return memoryChunk->checksum == calculateChecksum(memoryChunk);
	}
#endif

static struct MemoryChunk* getMemoryChunkFromAvailableListElement(struct DoubleLinkedListElement* listElement) {
	if (listElement != NULL) {
		uint32_t address = ((uint32_t) listElement) - offsetof(struct MemoryChunk, availableListElement);
		return (struct MemoryChunk*) address;
	} else {
		return NULL;
	}
}

static bool isAvailable(struct MemoryChunk* memoryChunk) {
	return (memoryChunk->flags & AVAILABLE_FLAG_MASK) != 0;
}

static void updateChecksum(struct MemoryChunk* memoryChunk) {
	#ifndef NDEBUG
		memoryChunk->checksum = calculateChecksum(memoryChunk);
	#endif
}

static struct MemoryChunk* findAvailableChunk(size_t size) {
	if (doubleLinkedListSize(&availableMemoryChunkList) == 0) {
		return NULL;

	} else {
		struct MemoryChunk* availableMemoryChunk = NULL;

		struct DoubleLinkedListElement* currentListElement = doubleLinkedListFirst(&availableMemoryChunkList);
		while (currentListElement != NULL) {
			struct MemoryChunk* currentMemoryChunk = getMemoryChunkFromAvailableListElement(currentListElement);
			assert(doesChecksumMatchs(currentMemoryChunk));
			assert(isAvailable(currentMemoryChunk));
			
			if (currentMemoryChunk->size >= size) {
				availableMemoryChunk = currentMemoryChunk;
				break;
			}
			
			currentListElement = currentListElement->next;
		}

		assert(availableMemoryChunk == NULL || doesChecksumMatchs(availableMemoryChunk));
		return availableMemoryChunk;
	}
}

static void removeFromAvailableList(struct MemoryChunk* memoryChunk) {
	#ifndef NDEBUG
		struct MemoryChunk* previous = getMemoryChunkFromAvailableListElement(memoryChunk->availableListElement.previous);
		struct MemoryChunk* next = getMemoryChunkFromAvailableListElement(memoryChunk->availableListElement.next);
	#endif

	doubleLinkedListRemove(&availableMemoryChunkList, &memoryChunk->availableListElement);
	#ifndef NDEBUG
		if (previous != NULL) {
			updateChecksum(previous);
		}
		if (next != NULL) {
			updateChecksum(next);
		}
	#endif
}

static void insertIntoAvailableList(struct MemoryChunk* memoryChunk) {
	#ifndef NDEBUG
		struct MemoryChunk* last = getMemoryChunkFromAvailableListElement(doubleLinkedListLast(&availableMemoryChunkList));
	#endif

	doubleLinkedListInsertAfterLast(&availableMemoryChunkList, &memoryChunk->availableListElement);
	#ifndef NDEBUG
		if (last != NULL) {
			updateChecksum(last);
		}
	#endif
}

static void insertIntoListAfter(struct MemoryChunk* memoryChunkBefore, struct MemoryChunk* memoryChunk) {
	#ifndef NDEBUG
		struct MemoryChunk* next;
		if (memoryChunkBefore == NULL) {
			next = (void*) doubleLinkedListFirst(&memoryChunkList);
		} else {
			next = (void*) memoryChunkBefore->listElement.next;
		}
	#endif

	if (memoryChunkBefore != NULL) {
		doubleLinkedListInsertAfter(&memoryChunkList, &memoryChunkBefore->listElement, &memoryChunk->listElement);
	} else {
		doubleLinkedListInsertBeforeFirst(&memoryChunkList, &memoryChunk->listElement);
	}
	#ifndef NDEBUG
		if (memoryChunkBefore != NULL) {
			updateChecksum(memoryChunkBefore);
		}
		if (next != NULL) {
			updateChecksum(next);
		}
	#endif
}

static void removeFromList(struct MemoryChunk* memoryChunk) {
	#ifndef NDEBUG
		struct MemoryChunk* previous = (void*) memoryChunk->listElement.previous;
		struct MemoryChunk* next = (void*) memoryChunk->listElement.next;
	#endif

	doubleLinkedListRemove(&memoryChunkList, &memoryChunk->listElement);
	#ifndef NDEBUG
		if (previous != NULL) {
			updateChecksum(previous);
		}
		if (next != NULL) {
			updateChecksum(next);
		}
	#endif
}

void simpleMemoryAllocatorInitialize(void* (*newSbrk)(int increment)) {
	doubleLinkedListInitialize(&memoryChunkList);
	doubleLinkedListInitialize(&availableMemoryChunkList);
	sbrk = newSbrk;
}

void* simpleMemoryAllocatorAcquire(size_t size) {
	/* It avoids obvious overflows. */
	if (size == 0 || size > LONG_MAX || size + sizeof(struct MemoryChunk) > LONG_MAX) {
		return NULL;

	} else {
		struct MemoryChunk* availableMemoryChunk = findAvailableChunk(size);
		if (availableMemoryChunk == NULL) {
			int increment = mathUtilsMax(size + sizeof(struct MemoryChunk), DATA_SEGMENT_MIN_INCREMENT);

			/* It needs to be initialized. */
			if (doubleLinkedListSize(&memoryChunkList) == 0) {
				struct MemoryChunk* newMemoryChunk;
				bool succeeded = false;
				void* sbrkResult;

				sbrkResult = sbrk(0);
				if (((void*) -1) != sbrkResult) {
					newMemoryChunk = sbrkResult;
					void* result = sbrk(increment);
					if (((void*) -1) != result) {
						insertIntoListAfter(NULL, newMemoryChunk);
						newMemoryChunk->size = increment - sizeof(struct MemoryChunk);
						newMemoryChunk->flags = AVAILABLE_FLAG_MASK;
						insertIntoAvailableList(newMemoryChunk);
						updateChecksum(newMemoryChunk);

						succeeded = true;
					}
				}

				if (!succeeded) {
					return NULL;

				} else {
					availableMemoryChunk = newMemoryChunk;
				}

			} else {
				/* It needs to call the kernel to allocate more memory. */
				struct MemoryChunk* lastMemoryChunk = (void*) doubleLinkedListLast(&memoryChunkList);
				void* result = sbrk(increment);
				if (((void*) -1) != result) {
					availableMemoryChunk = ((void*) lastMemoryChunk) + sizeof(struct MemoryChunk) + lastMemoryChunk->size;

					insertIntoListAfter(lastMemoryChunk, availableMemoryChunk);
					availableMemoryChunk->size = increment - sizeof(struct MemoryChunk);
					availableMemoryChunk->flags = AVAILABLE_FLAG_MASK;
					insertIntoAvailableList(availableMemoryChunk);
					updateChecksum(availableMemoryChunk);

				} else {
					return NULL;
				}
			}
		}
		assert(availableMemoryChunk != NULL);
		assert(doesChecksumMatchs(availableMemoryChunk));
		assert(isAvailable(availableMemoryChunk));

		removeFromAvailableList(availableMemoryChunk);

		if (availableMemoryChunk->size < size + sizeof(struct MemoryChunk) + MIN_CHUNK_SIZE) {
			availableMemoryChunk->flags &= ~AVAILABLE_FLAG_MASK;
			updateChecksum(availableMemoryChunk);

		} else {
			struct MemoryChunk* newMemoryChunk = ((void*) availableMemoryChunk) + sizeof(struct MemoryChunk) + size;

			insertIntoListAfter(availableMemoryChunk, newMemoryChunk);
			newMemoryChunk->flags = AVAILABLE_FLAG_MASK;
			newMemoryChunk->size = availableMemoryChunk->size - size - sizeof(struct MemoryChunk);
			assert(newMemoryChunk->size >= MIN_CHUNK_SIZE);
			insertIntoAvailableList(newMemoryChunk);
			updateChecksum(newMemoryChunk);

			availableMemoryChunk->size = size;
			availableMemoryChunk->flags &= ~AVAILABLE_FLAG_MASK;
			updateChecksum(availableMemoryChunk);
		}

		return ((void*) availableMemoryChunk) + sizeof(struct MemoryChunk);
	}
}

void simpleMemoryAllocatorRelease(void* pointer) {
	if (pointer != NULL) {
		struct MemoryChunk* memoryChunk = (pointer - sizeof(struct MemoryChunk));
		assert(doesChecksumMatchs(memoryChunk));
		assert(!isAvailable(memoryChunk));
		
		struct MemoryChunk* survivorMemoryChunk = NULL;
		struct MemoryChunk* previousMemoryChunk = (void*) memoryChunk->listElement.previous;
		if (previousMemoryChunk != NULL && isAvailable(previousMemoryChunk)) {
			struct MemoryChunk* previousPreviousMemoryChunk = (void*) previousMemoryChunk->listElement.previous;

			removeFromAvailableList(previousMemoryChunk);

			removeFromList(previousMemoryChunk);
			removeFromList(memoryChunk);

			survivorMemoryChunk = previousMemoryChunk;
			survivorMemoryChunk->size += sizeof(struct MemoryChunk) + memoryChunk->size;
			
			insertIntoListAfter(previousPreviousMemoryChunk, survivorMemoryChunk);

		} else {
			survivorMemoryChunk = memoryChunk;
			survivorMemoryChunk->flags |= AVAILABLE_FLAG_MASK;
		}
		assert(survivorMemoryChunk != NULL);
		assert(isAvailable(survivorMemoryChunk));
		
		struct MemoryChunk* nextMemoryChunk = (void*) survivorMemoryChunk->listElement.next;
		if (nextMemoryChunk != NULL && isAvailable(nextMemoryChunk)) {
			removeFromList(nextMemoryChunk);
			removeFromAvailableList(nextMemoryChunk);

			survivorMemoryChunk->size += sizeof(struct MemoryChunk) + nextMemoryChunk->size;
		}
		
		insertIntoAvailableList(survivorMemoryChunk);
		updateChecksum(survivorMemoryChunk);
		assert(isAvailable(survivorMemoryChunk));
	}
}

void* simpleMemoryAllocatorResize(void* pointer, size_t size) {
	if (size == 0) {
		simpleMemoryAllocatorRelease(pointer);
		return NULL;

	} else if (pointer == NULL) {
		return simpleMemoryAllocatorAcquire(size);

	} else {
		struct MemoryChunk* memoryChunk = (pointer - sizeof(struct MemoryChunk));
		assert(doesChecksumMatchs(memoryChunk));
		assert(!isAvailable(memoryChunk));

		if (memoryChunk->size > size) {
			/* It will be shrunk. */
			if (memoryChunk->size - size >= sizeof(struct MemoryChunk) + MIN_CHUNK_SIZE) {
				struct MemoryChunk* nextMemoryChunk = (void*) memoryChunk->listElement.next;
				struct MemoryChunk* newMemoryChunk = pointer + size;

				if (nextMemoryChunk != NULL && isAvailable(nextMemoryChunk)) {
					/* Merge with the next one. */

					removeFromList(nextMemoryChunk);

					removeFromAvailableList(nextMemoryChunk);

					newMemoryChunk->size = memoryChunk->size - size - sizeof(struct MemoryChunk) + sizeof(struct MemoryChunk) + nextMemoryChunk->size;

				} else {
					/* Create a new chunk. */
					assert(nextMemoryChunk == NULL || ((void*) newMemoryChunk) + sizeof(struct MemoryChunk)
							+ (memoryChunk->size - size - sizeof(struct MemoryChunk)) == nextMemoryChunk);
					newMemoryChunk->size = memoryChunk->size - size - sizeof(struct MemoryChunk);
				}

				insertIntoListAfter(memoryChunk, newMemoryChunk);

				memoryChunk->size = size;
				updateChecksum(memoryChunk);

				insertIntoAvailableList(newMemoryChunk);
				newMemoryChunk->flags = AVAILABLE_FLAG_MASK;
				updateChecksum(newMemoryChunk);
			}
			return pointer;

		} else if (memoryChunk->size < size) {
			/* It will be enlarged. */
			struct MemoryChunk* nextMemoryChunk = (void*) memoryChunk->listElement.next;
			if (nextMemoryChunk != NULL && isAvailable(nextMemoryChunk)
					&& size - memoryChunk->size <= sizeof(struct MemoryChunk) + nextMemoryChunk->size) {

				removeFromList(nextMemoryChunk);

				removeFromAvailableList(nextMemoryChunk);

				if (sizeof(struct MemoryChunk) + nextMemoryChunk->size - (size - memoryChunk->size) >= sizeof(struct MemoryChunk) + MIN_CHUNK_SIZE) {
					/* Create a new chunk. */
					size_t newMemoryChunkSize = nextMemoryChunk->size + sizeof(struct MemoryChunk) - (size - memoryChunk->size) - sizeof(struct MemoryChunk);

					struct MemoryChunk* newMemoryChunk = ((void*) memoryChunk) + sizeof(struct MemoryChunk) + size;
					insertIntoListAfter(memoryChunk, newMemoryChunk);
					newMemoryChunk->size = newMemoryChunkSize;
					newMemoryChunk->flags = AVAILABLE_FLAG_MASK;
					insertIntoAvailableList(newMemoryChunk);
					updateChecksum(newMemoryChunk);

					memoryChunk->size = size;

				} else {
					/* Use all space available. */

					if (memoryChunk->listElement.next != NULL) {
						updateChecksum((void*) memoryChunk->listElement.next);
					}

					memoryChunk->size += sizeof(struct MemoryChunk) + nextMemoryChunk->size;
				}

				updateChecksum(memoryChunk);
				return pointer;

			} else {
				void* newPointer = simpleMemoryAllocatorAcquire(size);
				if (newPointer != NULL) {
					memcpy(newPointer, pointer, memoryChunk->size);
					simpleMemoryAllocatorRelease(pointer);
					return newPointer;

				} else {
					return NULL;
				}
			}

			return NULL;

		} else {
			return pointer;
		}
	}
}

bool simpleMemoryAllocatorIsInternalStateValid(void) {
	bool isValid = true;
	struct MemoryChunk* memoryChunk = (void*) doubleLinkedListFirst(&memoryChunkList);

	while (memoryChunk != NULL) {
		#ifndef NDEBUG
			if (!doesChecksumMatchs(memoryChunk)) {
				isValid = false;
			}
		#endif

		memoryChunk = (void*) memoryChunk->listElement.next;
	}

	return isValid;
}

static void printMemoryChunk(struct MemoryChunk* memoryChunk, int (*printer)(const char* format, ...)) {
	printer("  memoryChunk=%p\n", memoryChunk);
	printer("  previousMemoryChunk=%p\n", memoryChunk->listElement.previous);
	printer("  nextMemoryChunk=%p\n", memoryChunk->listElement.next);

	if (isAvailable(memoryChunk)) {
		printer("  previousAvailableMemoryChunk=%p\n", memoryChunk->availableListElement.previous);
		printer("  nextAvailableMemoryChunk=%p\n", memoryChunk->availableListElement.next);
	}

	printer("  size=%u\n", memoryChunk->size);
	printer("  flags=%.4X\n", memoryChunk->flags);
	printer("  checksum=%.4X\n", memoryChunk->checksum);
	#ifndef NDEBUG
		printer("  expected checksum=%.4X\n", calculateChecksum(memoryChunk));
	#endif
	printer("  data=%p\n", ((void*) memoryChunk) + sizeof(struct MemoryChunk));
	printer("\n");
}

void simpleMemoryAllocatorPrintStatus(int (*printer)(const char* format, ...)) {
	uint32_t totalSize = 0;
	
	printf("memoryChunkList:\n");

	struct MemoryChunk* memoryChunk = (void*) doubleLinkedListFirst(&memoryChunkList);
	printer("  firstMemoryChunk=%p\n", memoryChunk);
	printer("\n");

	while (memoryChunk != NULL) {
		printMemoryChunk(memoryChunk, printer);
		
		totalSize += sizeof(struct MemoryChunk);
		totalSize += memoryChunk->size;
		memoryChunk = (void*) memoryChunk->listElement.next;
	}
	
	printer("  lastMemoryChunk=%p\n", (void*) doubleLinkedListLast(&memoryChunkList));
	printer("\n");

	printer("  totalSize=%u\n", totalSize);

	printf("availableMemoryChunkList:\n");

	struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(&availableMemoryChunkList);
	while (listElement != NULL) {
		printMemoryChunk(getMemoryChunkFromAvailableListElement(listElement), printer);

		listElement = listElement->next;
	}
}
