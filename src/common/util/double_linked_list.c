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
#include <stdlib.h>
#include "util/double_linked_list.h"

void doubleLinkedListInitialize(struct DoubleLinkedList* list) {
	list->size = 0;
	list->first = NULL;
	list->last = NULL;
}

void doubleLinkedListInsertBefore(struct DoubleLinkedList* list, struct DoubleLinkedListElement* elementAfter,
		struct DoubleLinkedListElement* element) {
	if (list->first == elementAfter) {
		assert(elementAfter->previous == NULL);
		list->first = element;

	} else {
		assert(elementAfter->previous != NULL);
		elementAfter->previous->next = element;
	}

	element->previous = elementAfter->previous;
	element->next = elementAfter;
	elementAfter->previous = element;

	list->size++;
}

void doubleLinkedListInsertAfter(struct DoubleLinkedList* list, struct DoubleLinkedListElement* elementBefore,
		struct DoubleLinkedListElement* element) {
	if (list->last == elementBefore) {
		assert(elementBefore->next == NULL);
		list->last = element;

	} else {
		assert(elementBefore->next != NULL);
		elementBefore->next->previous = element;
	}

	element->next = elementBefore->next;
	element->previous = elementBefore;
	elementBefore->next = element;

	list->size++;
}

void doubleLinkedListInsertBeforeFirst(struct DoubleLinkedList* list, struct DoubleLinkedListElement* element) {
	if (list->size > 0) {
		list->first->previous = element;
	} else {
		list->last = element;
	}
	element->next = list->first;
	element->previous = NULL;
	list->first = element;

	list->size++;
}

void doubleLinkedListInsertAfterLast(struct DoubleLinkedList* list, struct DoubleLinkedListElement* element) {
	if (list->size > 0) {
		list->last->next = element;
	} else {
		list->first = element;
	}
	element->previous = list->last;
	element->next = NULL;
	list->last = element;

	list->size++;
}

void doubleLinkedListRemove(struct DoubleLinkedList* list, struct DoubleLinkedListElement* element) {
	assert(list->size > 0);

	list->size--;
	if (list->size > 0) {
		if (list->first == element) {
			list->first = element->next;
			element->next->previous = NULL;

		} else if (list->last == element) {
			element->previous->next = NULL;
			list->last = element->previous;

		} else {
			element->next->previous = element->previous;
			element->previous->next = element->next;
		}

	} else {
		list->first = NULL;
		list->last = NULL;
	}

	element->next = NULL;
	element->previous = NULL;
}

bool doubleLinkedListContainsFoward(struct DoubleLinkedList* list, struct DoubleLinkedListElement* element) {
	struct DoubleLinkedListElement* currentDoubleLinkedListElement = list->first;
	while (currentDoubleLinkedListElement) {
		if (currentDoubleLinkedListElement == element) {
			return true;
		} else {
			currentDoubleLinkedListElement = currentDoubleLinkedListElement->next;
		}
	}

	return false;
}

bool doubleLinkedListContainsBackward(struct DoubleLinkedList* list, struct DoubleLinkedListElement* element) {
	struct DoubleLinkedListElement* currentDoubleLinkedListElement = list->last;
	while (currentDoubleLinkedListElement) {
		if (currentDoubleLinkedListElement == element) {
			return true;
		} else {
			currentDoubleLinkedListElement = currentDoubleLinkedListElement->previous;
		}
	}

	return false;
}

static void mergeSort(struct DoubleLinkedList* list, int (*compare)(struct DoubleLinkedListElement*, struct DoubleLinkedListElement*)) {
	int size = list->size;
	if (size == 2) {
		if (compare(list->first, list->last) >= 1) {
			struct DoubleLinkedListElement* old = list->first;

			list->first = list->last;
			list->first->previous = NULL;
			list->first->next = old;

			list->last = old;
			list->last->previous = list->first;
			list->last->next = NULL;
		}

	} else if (size >= 3) {
		struct DoubleLinkedListElement* median = list->first;
		for (int i = 0; i < (size / 2) - 1; i++) {
			median = median->next;
		}
		assert(median != NULL);

		struct DoubleLinkedListElement* medianNext = median->next;
		assert(medianNext != NULL);

		struct DoubleLinkedList list1;
		list1.size = size / 2;
		list1.first = list->first;
		assert(list1.first->previous == NULL);
		list1.last = median;
		median->next = NULL;

		struct DoubleLinkedList list2;
		list2.size = size - list1.size;
		list2.first = medianNext;
		list2.last = list->last;
		medianNext->previous = NULL;

		mergeSort(&list1, compare);
		mergeSort(&list2, compare);

		doubleLinkedListInitialize(list);
		struct DoubleLinkedListElement* next1 = list1.first;
		struct DoubleLinkedListElement* next2 = list2.first;
		while (next1 != NULL || next2 != NULL) {
			struct DoubleLinkedListElement* element;
			if (next1 != NULL && next2 != NULL) {
				if (compare(next1, next2) >= 1) {
					element = next2;
					next2 = next2->next;
				} else {
					element = next1;
					next1 = next1->next;
				}

			} else if (next1 != NULL) {
				element = next1;
				next1 = next1->next;
			} else {
				assert(next2 != NULL);
				element = next2;
				next2 = next2->next;
			}

			doubleLinkedListInsertAfterLast(list, element);
		}
	}
}

void doubleLinkedListSort(struct DoubleLinkedList* list, int (*compare)(struct DoubleLinkedListElement*, struct DoubleLinkedListElement*)) {
	mergeSort(list, compare);
}

void doubleLinkedListInsertListAfterLast(struct DoubleLinkedList* list1, struct DoubleLinkedList* list2) {
	if (list1->last != NULL) {
		if (list2->first != NULL) {
			assert(list2->first->previous == NULL);
			list2->first->previous = list1->last;

			assert(list1->last->next == NULL);
			list1->last->next = list2->first;

			list1->last = list2->last;
		}

	} else if (list2->first != NULL) {
		assert(list1->last == NULL);
		list1->first = list2->first;
		list1->last = list2->last;
	}

	list1->size += list2->size;

	list2->size = 0;
	list2->first = NULL;
	list2->last = NULL;
}

struct DoubleLinkedListElement* doubleLinkedListRemoveFirst(struct DoubleLinkedList* list) {
	struct DoubleLinkedListElement* element = list->first;
	if (element != NULL) {
		list->first = element->next;
		if (list->first != NULL) {
			list->first->previous = NULL;
		} else {
			list->last = NULL;
		}
		list->size--;
	}
	return element;
}

struct DoubleLinkedListElement* doubleLinkedListRemoveLast(struct DoubleLinkedList* doubleLinkedList) {
	struct DoubleLinkedListElement* element = doubleLinkedList->last;
	if (element != NULL) {
		doubleLinkedList->last = element->previous;
		if (doubleLinkedList->last != NULL) {
			doubleLinkedList->last->next = NULL;
		} else {
			doubleLinkedList->first = NULL;
		}
		doubleLinkedList->size--;
	}
	return element;
}

static bool hasNext(struct Iterator* iterator) {
	struct DoubleLinkedListIterator* doubleLinkedListIterator = (void*) iterator;
	return doubleLinkedListIterator->next != NULL;
}

static void* next(struct Iterator* iterator) {
	struct DoubleLinkedListIterator* doubleLinkedListIterator = (void*) iterator;
	void* result = doubleLinkedListIterator->next;
	doubleLinkedListIterator->next = doubleLinkedListIterator->next->next;
	return result;
}

void doubleLinkedListInitializeIterator(struct DoubleLinkedList* list, struct DoubleLinkedListIterator* iterator) {
	iterator->iterator.hasNext = &hasNext;
	iterator->iterator.next = &next;
	iterator->iterator.transformValueBeforeNextReturn = NULL;

	iterator->next = list->first;
}
