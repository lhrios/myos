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

#include "util/double_linked_list.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>

struct Integer {
	struct DoubleLinkedListElement doubleLinkedListElement;
	int value;
};

#define INTEGER_ARRAY_LENGTH 1024
struct Integer integers[INTEGER_ARRAY_LENGTH];

static int compare(struct DoubleLinkedListElement* doubleLinkedListElement1, struct DoubleLinkedListElement* doubleLinkedListElement2) {
	struct Integer* integer1 = (void*) doubleLinkedListElement1;
	struct Integer* integer2 = (void*) doubleLinkedListElement2;
	return integer1->value - integer2->value;
}

static void test1(void) {
	struct DoubleLinkedList list;

	doubleLinkedListInitialize(&list);
	for (int i = 0; i < INTEGER_ARRAY_LENGTH; i++) {
		assert(!doubleLinkedListContainsFoward(&list, &(integers[i].doubleLinkedListElement)));
		assert(!doubleLinkedListContainsBackward(&list, &(integers[i].doubleLinkedListElement)));
	}
	assert(doubleLinkedListSize(&list) == 0);

	int values[] = {10, 41, 69, 100, -5, 92, 2, 67, 2};

	/* It inserts the items in a backward fashion. */
	for (int i = 0; i < sizeof(values) / sizeof(int); i++) {
		struct Integer* integer = &integers[i];
		integer->value = values[i];

		doubleLinkedListInsertBeforeFirst(&list, &integer->doubleLinkedListElement);
	}
	assert(doubleLinkedListSize(&list) == sizeof(values) / sizeof(int));
	for (int i = 0; i < sizeof(values) / sizeof(int); i++) {
		struct Integer* integer = &integers[i];
		integer->value = values[i];

		assert(doubleLinkedListContainsFoward(&list, &(integers[i].doubleLinkedListElement)));
		assert(doubleLinkedListContainsBackward(&list, &(integers[i].doubleLinkedListElement)));
	}
	int i = (sizeof(values) / sizeof(int)) - 1;
	struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(&list);
	while (listElement) {
		struct Integer* integer = (struct Integer*) listElement;
		listElement = listElement->next;

		assert(integer->value == values[i]);
		i--;
	}

	/* It now remove each item. */
	for (int i = 0; i < sizeof(values) / sizeof(int); i++) {
		assert(doubleLinkedListContainsFoward(&list, &(integers[i].doubleLinkedListElement)));
		assert(doubleLinkedListContainsBackward(&list, &(integers[i].doubleLinkedListElement)));

		struct Integer* integer = (struct Integer*) doubleLinkedListLast(&list);
		assert(values[i] == integer->value);

		doubleLinkedListRemove(&list, doubleLinkedListLast(&list));
	}
}

static void test2(void) {
	struct DoubleLinkedList list;

	doubleLinkedListInitialize(&list);
	int values[] = {10, 41, 69, 100, -5, 92, 2, 67, 2};
	int sortedValues[] = {-5, 2, 2, 10, 41, 67, 69, 92, 100};
	assert(sizeof(values) == sizeof(sortedValues));

	/* It inserts the items in a foward fashion. */
	for (int i = 0; i < sizeof(values) / sizeof(int); i++) {
		struct Integer* integer = &integers[i];
		integer->value = sortedValues[i];

		doubleLinkedListInsertAfterLast(&list, &integer->doubleLinkedListElement);
	}

	/* It now remove each item. */
	for (int i = 0; i < sizeof(values) / sizeof(int); i++) {
		assert(doubleLinkedListSize(&list) == sizeof(values) / sizeof(int) - i);

		/* It finds the item that will be removed. */
		struct DoubleLinkedListElement* currentDoubleLinkedListElement = doubleLinkedListLast(&list);
		while (currentDoubleLinkedListElement) {
			struct Integer* integer = (struct Integer*) currentDoubleLinkedListElement;
			if (integer->value == values[i]) {
				break;
			} else {
				currentDoubleLinkedListElement = currentDoubleLinkedListElement->previous;
			}
		}
		doubleLinkedListRemove(&list, currentDoubleLinkedListElement);
	}

	assert(doubleLinkedListSize(&list) == 0);
}

static void test3(void) {
	struct DoubleLinkedList list;

	doubleLinkedListInitialize(&list);
	int values[] = {10, 41, 69, 100, -5, 92, 2, 67, 2};

	/* It inserts the items in a foward fashion. */
	for (int i = 0; i < sizeof(values) / sizeof(int); i++) {
		struct Integer* integer = &integers[i];
		integer->value = values[i];

		doubleLinkedListInsertAfterLast(&list, &integer->doubleLinkedListElement);
	}

	int i = 0;
	while (doubleLinkedListSize(&list) != 0) {
		assert(doubleLinkedListSize(&list) == sizeof(values) / sizeof(int) - i);

		struct DoubleLinkedListElement* doubleLinkedListElement1 = doubleLinkedListFirst(&list);
		struct DoubleLinkedListElement* doubleLinkedListElement2 = doubleLinkedListRemoveFirst(&list);

		assert(doubleLinkedListElement1 == doubleLinkedListElement2);
		struct Integer* integer = (struct Integer*) doubleLinkedListElement1;
		assert(values[i] == integer->value);

		i++;
	}
}

static void test4(void) {
	struct DoubleLinkedList list;
	doubleLinkedListInitialize(&list);

	int values[] = {10, 41, 69, 100, -5, 92, 2, 67, 2, 33};

	/* It inserts the items in a foward fashion. */
	for (int i = 0; i < sizeof(values) / sizeof(int); i++) {
		struct Integer* integer = &integers[i];
		integer->value = values[i];

		doubleLinkedListInsertAfterLast(&list, &integer->doubleLinkedListElement);
	}

	doubleLinkedListSort(&list, &compare);

	int previousValue = INT_MIN;
	struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(&list);
	while (listElement != NULL) {
		struct Integer* integer = (void*) listElement;
		listElement = listElement->next;

		assert(previousValue <= integer->value);
		previousValue = integer->value;
	}
}

static void test5(void) {
	struct DoubleLinkedList list;

	doubleLinkedListInitialize(&list);
	int values[] = {10, 20, 30, 40, 50};

	/* It inserts the items in a foward fashion. */
	for (int i = 0; i < sizeof(values) / sizeof(int); i++) {
		struct Integer* integer = &integers[i];
		integer->value = values[i];

		doubleLinkedListInsertAfterLast(&list, &integer->doubleLinkedListElement);
	}

	struct DoubleLinkedListIterator doubleLinkedListIterator;
	doubleLinkedListInitializeIterator(&list, &doubleLinkedListIterator);
	struct Iterator* iterator = &doubleLinkedListIterator.iterator;

	int index = 0;
	while (iteratorHasNext(iterator)) {
		struct Integer* integer = iteratorNext(iterator);
		assert(values[index] == integer->value);
		index++;
	}
	assert(index == sizeof(values) / sizeof(int));
}

static void test6(void) {
	struct DoubleLinkedList list;

	doubleLinkedListInitialize(&list);
	int values[] = {10, 20, 30, 40, 50, 60};

	struct Integer* integer20 = &integers[1];
	integer20->value = 20;
	doubleLinkedListInsertBeforeFirst(&list, &integer20->doubleLinkedListElement);

	struct Integer* integer50 = &integers[4];
	integer50->value = 50;
	doubleLinkedListInsertAfterLast(&list, &integer50->doubleLinkedListElement);

	struct Integer* integer10 = &integers[0];
	integer10->value = 10;
	doubleLinkedListInsertBefore(&list, &integer20->doubleLinkedListElement, &integer10->doubleLinkedListElement);

	struct Integer* integer60 = &integers[5];
	integer60->value = 60;
	doubleLinkedListInsertAfter(&list, &integer50->doubleLinkedListElement, &integer60->doubleLinkedListElement);

	struct Integer* integer30 = &integers[2];
	integer30->value = 30;
	doubleLinkedListInsertAfter(&list, &integer20->doubleLinkedListElement, &integer30->doubleLinkedListElement);

	struct Integer* integer40 = &integers[3];
	integer40->value = 40;
	doubleLinkedListInsertBefore(&list, &integer50->doubleLinkedListElement, &integer40->doubleLinkedListElement);

	assert(doubleLinkedListSize(&list) == 6);
	
	struct DoubleLinkedListElement* element = &integer10->doubleLinkedListElement;
	const int valueCount = sizeof(values) / sizeof(int);
	for (int i = 0; i < valueCount; i++) {
		struct Integer* integer = (void*) element;
		assert(element != NULL);
		assert(integer == &integers[i]);
		assert(integer->value == values[i]);

		if (i == 0) {
			assert(element == doubleLinkedListFirst(&list));
			assert(element->previous == NULL);
			assert(element->next == (void*) &integers[i + 1]);

		} else if (i == valueCount - 1) {
			assert(element == doubleLinkedListLast(&list));
			assert(element->previous == (void*) &integers[i - 1]);
			assert(element->next == NULL);

		} else {
			assert(element->previous == (void*) &integers[i - 1]);
			assert(element->next == (void*) &integers[i + 1]);
		}

		element = element->next;
	}
}

int main(int argc, char** argv) {
	test1();
	test2();
	test3();
	test4();
	test5();
	test6();

	return 0;
}
