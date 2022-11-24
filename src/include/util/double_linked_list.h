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

#ifndef DOUBLE_LINKED_LIST_H
	#define DOUBLE_LINKED_LIST_H

	#include <stdbool.h>

	#include "util/iterator.h"

	struct DoubleLinkedListElement {
		struct DoubleLinkedListElement* next;
		struct DoubleLinkedListElement* previous;
	};

	struct DoubleLinkedList {
		int size;
		struct DoubleLinkedListElement* first;
		struct DoubleLinkedListElement* last;
	};

	struct DoubleLinkedListIterator {
		struct Iterator iterator;
		struct DoubleLinkedListElement* next;
	};

	inline __attribute__((always_inline)) int doubleLinkedListSize(struct DoubleLinkedList* list) {
		return list->size;
	}

	inline __attribute__((always_inline)) struct DoubleLinkedListElement* doubleLinkedListFirst(struct DoubleLinkedList* list) {
		return list->first;
	}

	inline __attribute__((always_inline)) struct DoubleLinkedListElement* doubleLinkedListLast(
			struct DoubleLinkedList* doubleLinkedList) {
		return doubleLinkedList->last;
	}

	void doubleLinkedListInitialize(struct DoubleLinkedList* doubleLinkedList);

	struct DoubleLinkedListElement* doubleLinkedListRemoveFirst(struct DoubleLinkedList* list);

	struct DoubleLinkedListElement* doubleLinkedListRemoveLast(struct DoubleLinkedList* doubleLinkedList);

	void doubleLinkedListInsertListAfterLast(struct DoubleLinkedList* list1, struct DoubleLinkedList* list2);

	void doubleLinkedListInsertBeforeFirst(struct DoubleLinkedList* list, struct DoubleLinkedListElement* element);

	void doubleLinkedListInsertAfterLast(struct DoubleLinkedList* list, struct DoubleLinkedListElement* element);

	void doubleLinkedListRemove(struct DoubleLinkedList* list, struct DoubleLinkedListElement* element);

	bool doubleLinkedListContainsFoward(struct DoubleLinkedList* list, struct DoubleLinkedListElement* element);

	bool doubleLinkedListContainsBackward(struct DoubleLinkedList* list, struct DoubleLinkedListElement* element);

	void doubleLinkedListSort(struct DoubleLinkedList* list, int (*compare)(struct DoubleLinkedListElement*, struct DoubleLinkedListElement*));

	void doubleLinkedListInitializeIterator(struct DoubleLinkedList* list, struct DoubleLinkedListIterator* iterator);

	void doubleLinkedListInsertBefore(struct DoubleLinkedList* list, struct DoubleLinkedListElement* elementAfter,
			struct DoubleLinkedListElement* element);

	void doubleLinkedListInsertAfter(struct DoubleLinkedList* list, struct DoubleLinkedListElement* elementBefore,
			struct DoubleLinkedListElement* element);

#endif
