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

#ifndef UNROLLED_LINKED_LIST_H
	#define UNROLLED_LINKED_LIST_H

	#include <stdbool.h>
	#include <stddef.h>

	#include "util/iterator.h"

	/*
	 * References:
	 * - Unrolled linked list - https://en.wikipedia.org/wiki/Unrolled_linked_list
	 */

	struct UnrolledLinkedListNode {
		struct UnrolledLinkedListNode* next;
	};

	struct UnrolledLinkedList {
		int size;
		struct UnrolledLinkedListNode* first;
		struct UnrolledLinkedListNode* last;
		size_t elementSize;
		size_t nodeSize;
		size_t maxElementsPerNode;
		void* memoryAllocatorContext;
		void* (*memoryAllocatorAcquire)(void*, size_t);
		void (*memoryAllocatorRelease)(void*, void*);
	};

	struct UnrolledLinkedListIterator {
		struct Iterator iterator;
		struct UnrolledLinkedList* list;
		struct UnrolledLinkedListNode* node;
		int index;
	};

	void unrolledLinkedListInitialize(struct UnrolledLinkedList* list, size_t nodeSize, size_t elementSize,
			void* memoryAllocatorContext, void* (*memoryAllocatorAcquire)(void*, size_t), void (*memoryAllocatorRelease)(void*, void*));
	void unrolledLinkedListGet(struct UnrolledLinkedList* list, int index, void* element);
	bool unrolledLinkedListInsertAfterLast(struct UnrolledLinkedList* list, void* element);
	void unrolledLinkedListClear(struct UnrolledLinkedList* list);
	void unrolledLinkedListInitializeIterator(struct UnrolledLinkedList* list, struct UnrolledLinkedListIterator* iterator);

	inline __attribute__((always_inline)) int unrolledLinkedListSize(struct UnrolledLinkedList* list) {
		return list->size;
	}

#endif
