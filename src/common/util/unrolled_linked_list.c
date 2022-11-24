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

#include <stdio.h>

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "util/unrolled_linked_list.h"

static void* getElementAddress(struct UnrolledLinkedList* list, struct UnrolledLinkedListNode* node, int index) {
	return (void*) &node->next + sizeof(void*) + (index * list->elementSize);
}

void unrolledLinkedListInitialize(struct UnrolledLinkedList* list, size_t nodeSize, size_t elementSize,
		void* memoryAllocatorContext, void* (*memoryAllocatorAcquire)(void*, size_t), void (*memoryAllocatorRelease)(void*, void*)) {

	memset(list, 0, sizeof(struct UnrolledLinkedList));

	assert(nodeSize > sizeof(struct UnrolledLinkedListNode));
	list->maxElementsPerNode = (nodeSize - sizeof(struct UnrolledLinkedListNode)) / elementSize;
	assert(list->maxElementsPerNode > 0);

	list->elementSize = elementSize;
	list->nodeSize = nodeSize;
	list->memoryAllocatorContext = memoryAllocatorContext;
	list->memoryAllocatorAcquire = memoryAllocatorAcquire;
	list->memoryAllocatorRelease = memoryAllocatorRelease;
}

void unrolledLinkedListGet(struct UnrolledLinkedList* list, int index, void* element) {
	assert(0 <= index && index < list->elementSize);

	int nodeIndex = index / list->maxElementsPerNode;
	struct UnrolledLinkedListNode* node = list->first;
	while (nodeIndex > 0) {
		node = node->next;
		nodeIndex--;
	}

	memcpy(element, getElementAddress(list, node, index % list->maxElementsPerNode), list->elementSize);
}

bool unrolledLinkedListInsertAfterLast(struct UnrolledLinkedList* list, void* element) {
	if (list->first == NULL) {
		assert(list->size == 0);

		struct UnrolledLinkedListNode* node = list->memoryAllocatorAcquire(list->memoryAllocatorContext, list->nodeSize);
		if (node != NULL) {
			void* elementAddress = getElementAddress(list, node, 0);
			memcpy(elementAddress, element, list->elementSize);

			node->next = NULL;
			list->first = node;
			list->last = node;
			list->size++;

			return true;
		}

	} else {
		/* Is there enough space left? */
		if (list->size % list->maxElementsPerNode != 0) {
			void* elementAddress = getElementAddress(list, list->last, list->size % list->maxElementsPerNode);
			memcpy(elementAddress, element, list->elementSize);

			list->size++;

			return true;

		} else {
			struct UnrolledLinkedListNode* node = list->memoryAllocatorAcquire(list->memoryAllocatorContext, list->nodeSize);
			if (node != NULL) {
				void* elementAddress = getElementAddress(list, node, 0);
				memcpy(elementAddress, element, list->elementSize);

				node->next = NULL;
				list->last->next = node;
				list->last = node;
				list->size++;

				return true;
			}
		}
	}

	return false;
}

void unrolledLinkedListClear(struct UnrolledLinkedList* list) {
	struct UnrolledLinkedListNode* node = list->first;
	while (node != NULL) {
		struct UnrolledLinkedListNode* next = node->next;
		list->memoryAllocatorRelease(list->memoryAllocatorContext, node);
		node = next;
	}

	list->first = NULL;
	list->last = NULL;
	list->size = 0;
}

static bool hasNext(struct UnrolledLinkedListIterator* iterator) {
	struct UnrolledLinkedList* list = iterator->list;
	return iterator->index < list->size;
}

static void* next(struct UnrolledLinkedListIterator* iterator) {
	struct UnrolledLinkedList* list = iterator->list;
	struct UnrolledLinkedListNode* node = iterator->node;
	int index = iterator->index++;
	if (index % list->maxElementsPerNode == 0) {
		if (node == NULL) {
			iterator->node = list->first;
			node = iterator->node;
		} else {
			iterator->node = node->next;
			node = iterator->node;
		}
		assert(node != NULL);
		return getElementAddress(list, node, 0);

	} else {
		return getElementAddress(list, node, index % list->maxElementsPerNode);
	}
}

void unrolledLinkedListInitializeIterator(struct UnrolledLinkedList* list, struct UnrolledLinkedListIterator* iterator) {
	iterator->iterator.hasNext = (bool (*)(struct Iterator*)) &hasNext;
	iterator->iterator.next = (void* (*)(struct Iterator*)) &next;
	iterator->iterator.transformValueBeforeNextReturn = NULL;

	iterator->list = list;
	iterator->node = NULL;
	iterator->index = 0;
}
