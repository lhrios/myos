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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/priority_queue.h"

static void* getElementAddress(struct PriorityQueue* priorityQueue, int index) {
	return priorityQueue->queue + (index * priorityQueue->elementSize);
}

static void swap(struct PriorityQueue* priorityQueue, void* element1, void* element2) {
	uint8_t* castedElement1 = element1;
	uint8_t* castedElement2 = element2;

	for (int i = 0; i < priorityQueue->elementSize; i++) {
		uint8_t temp = castedElement1[i];
		castedElement1[i] = castedElement2[i];
		castedElement2[i] = temp;
	}
}

static void heapifyUp(struct PriorityQueue* priorityQueue, int index) {
	bool (*comparator)(const void* a, const void* b, void* argument1, void* argument2, void* argument3) = priorityQueue->comparator;
	void* argument1 = priorityQueue->comparatorArgument1;
	void* argument2 = priorityQueue->comparatorArgument2;
	void* argument3 = priorityQueue->comparatorArgument3;

	while (index > 0) {
		int parentIndex;
		if (index % 2 == 0) {
			parentIndex = index - 2;
		} else {
			parentIndex = index - 1;
		}
		parentIndex /= 2;

		void* parent = getElementAddress(priorityQueue, parentIndex);
		void* child = getElementAddress(priorityQueue, index);
		if (!comparator(parent, child, argument1, argument2, argument3)) {
			swap(priorityQueue, parent, child);
			index = parentIndex;

		} else {
			break;
		}
	}
}

static void heapifyDown(struct PriorityQueue* priorityQueue, int index) {
	bool (*comparator)(const void* a, const void* b, void* argument1, void* argument2, void* argument3) = priorityQueue->comparator;
	void* argument1 = priorityQueue->comparatorArgument1;
	void* argument2 = priorityQueue->comparatorArgument2;
	void* argument3 = priorityQueue->comparatorArgument3;

	while (true) {
		int child1Index = index * 2 + 1;
		int child2Index = index * 2 + 2;

		int smallestItemIndex = index;

		if (child1Index < priorityQueue->size) {
			void* child1 = getElementAddress(priorityQueue, child1Index);
			void* smallestItem = getElementAddress(priorityQueue, smallestItemIndex);

			if (comparator(child1, smallestItem, argument1, argument2, argument3)) {
				smallestItemIndex = child1Index;
			}
		}

		if (child2Index < priorityQueue->size) {
			void* child2 = getElementAddress(priorityQueue, child2Index);
			void* smallestItem = getElementAddress(priorityQueue, smallestItemIndex);

			if (comparator(child2, smallestItem, argument1, argument2, argument3)) {
				smallestItemIndex = child2Index;
			}
		}

		if (smallestItemIndex != index) {
			swap(priorityQueue, getElementAddress(priorityQueue, smallestItemIndex), getElementAddress(priorityQueue, index));
			index = smallestItemIndex;

		} else {
			break;
		}
	}
}

void priorityQueuePeek(struct PriorityQueue* priorityQueue, void* item) {
	memcpy(item, getElementAddress(priorityQueue, 0), priorityQueue->elementSize);
}

void priorityQueueInitialize(struct PriorityQueue* priorityQueue, void* queue, size_t capacity, size_t initialSize, size_t elementSize,
		bool (*comparator)(const void*, const void*, void*, void*, void*),
		void* comparatorArgument1, void* comparatorArgument2, void* comparatorArgument3) {
	priorityQueue->capacity = capacity;
	priorityQueue->size = initialSize;
	priorityQueue->queue = queue;
	priorityQueue->elementSize = elementSize;
	priorityQueue->comparator = comparator;
	priorityQueue->comparatorArgument1 = comparatorArgument1;
	priorityQueue->comparatorArgument2 = comparatorArgument2;
	priorityQueue->comparatorArgument3 = comparatorArgument3;

	if (initialSize > 0) {
		for (int index = initialSize - 1; index >= 0; index--) {
			heapifyDown(priorityQueue, index);
		}
	}
}

bool priorityQueueInsert(struct PriorityQueue* priorityQueue, void* item) {
	if (priorityQueue->size + 1 > priorityQueue->capacity) {
		return false;

	} else {
		memcpy(getElementAddress(priorityQueue, priorityQueue->size), item, priorityQueue->elementSize);
		heapifyUp(priorityQueue, priorityQueue->size);

		priorityQueue->size++;

		return true;
	}
}

void priorityQueueRemove(struct PriorityQueue* priorityQueue, void* item) {
	memcpy(item, getElementAddress(priorityQueue, 0), priorityQueue->elementSize);

	priorityQueue->size--;
	if (priorityQueue->size > 0) {
		swap(priorityQueue, getElementAddress(priorityQueue, 0), getElementAddress(priorityQueue, priorityQueue->size));
		heapifyDown(priorityQueue, 0);
	}
}

static bool reverseComparator(const void* item1, const void* item2, void* comparator, void* hasArgument, void* argument) {
	if (hasArgument) {
		return ((int (*)(const void*, const void*, void*)) comparator)(item1, item2, argument) > 0;

	} else {
		return ((int (*)(const void*, const void*)) comparator)(item1, item2) > 0;
	}
}

void priorityQueueInplaceArraySort(void* array, size_t arrayLength, size_t elementSize, void *comparator, bool comparatorHasArgument, void* comparatorArgument) {
	struct PriorityQueue priorityQueue;
	priorityQueueInitialize(&priorityQueue, array, arrayLength, arrayLength, elementSize,
			&reverseComparator,
			comparator, (void*) (int) comparatorHasArgument, comparatorArgument);

	for (int i = arrayLength - 1; i > 0; i--) {
		swap(&priorityQueue, getElementAddress(&priorityQueue, i), getElementAddress(&priorityQueue, 0));
		priorityQueue.size--;
		heapifyDown(&priorityQueue, 0);
	}
}
