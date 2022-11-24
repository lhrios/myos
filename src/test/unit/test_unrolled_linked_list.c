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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/unrolled_linked_list.h"

static void* memoryAllocatorAcquire(void* context, size_t size) {
	return malloc(size);
}

static void memoryAllocatorRelease(void* context, void* pointer) {
	free(pointer);
}

#define KEY_LENGTH 32
struct Element {
	char value[KEY_LENGTH];
};

static void test1(void) {
	const char* VALUES[] = {
		"color",
		"water",
		"value",
		"name",
		"blue"
	};

	struct UnrolledLinkedList list;

	unrolledLinkedListInitialize(&list, 128, sizeof(struct Element),
			NULL, &memoryAllocatorAcquire, &memoryAllocatorRelease);
	assert(unrolledLinkedListSize(&list) == 0);

	bool result;
	struct Element element;

	strcpy(element.value, VALUES[0]);
	result = unrolledLinkedListInsertAfterLast(&list, &element);
	assert(result);

	strcpy(element.value, VALUES[1]);
	result = unrolledLinkedListInsertAfterLast(&list, &element);
	assert(result);

	strcpy(element.value, VALUES[2]);
	result = unrolledLinkedListInsertAfterLast(&list, &element);
	assert(result);

	strcpy(element.value, VALUES[3]);
	result = unrolledLinkedListInsertAfterLast(&list, &element);
	assert(result);

	strcpy(element.value, VALUES[4]);
	result = unrolledLinkedListInsertAfterLast(&list, &element);
	assert(result);

	assert(unrolledLinkedListSize(&list) == 5);

	unrolledLinkedListGet(&list, 0, &element);
	assert(strcmp(element.value, VALUES[0]) == 0);

	unrolledLinkedListGet(&list, 1, &element);
	assert(strcmp(element.value, VALUES[1]) == 0);

	unrolledLinkedListGet(&list, 2, &element);
	assert(strcmp(element.value, VALUES[2]) == 0);

	unrolledLinkedListGet(&list, 3, &element);
	assert(strcmp(element.value, VALUES[3]) == 0);

	unrolledLinkedListGet(&list, 4, &element);
	assert(strcmp(element.value, VALUES[4]) == 0);

	int index = 0;
	struct UnrolledLinkedListIterator iterator;
	unrolledLinkedListInitializeIterator(&list, &iterator);
	while (iteratorHasNext(&iterator.iterator)) {
		struct Element* element = iteratorNext(&iterator.iterator);
		assert(strcmp(VALUES[index++], element->value) == 0);
	}

	unrolledLinkedListClear(&list);
}

static void test2(void) {
	const int BUFFER_SIZE = 256;
	char buffer[BUFFER_SIZE];

	struct UnrolledLinkedList list;

	unrolledLinkedListInitialize(&list, 4096, sizeof(struct Element),
			NULL, &memoryAllocatorAcquire, &memoryAllocatorRelease);
	assert(unrolledLinkedListSize(&list) == 0);

	int wordCount = 0;
	FILE* file = fopen("../resources/test/unit/words.txt", "r");
	if (file != NULL) {
		while (!feof(file)) {
			if (NULL != fgets(buffer, BUFFER_SIZE, file)) {
				size_t length = strlen(buffer);
				if (length > 0) {
					if (buffer[length - 1] == '\n') {
						buffer[length - 1] = '\0';
						length--;
					}
				}

				if (0 <= length && length < KEY_LENGTH) {
					struct Element element;
					strcpy(element.value, buffer);
					bool result = unrolledLinkedListInsertAfterLast(&list, &element);
					assert(result);
					wordCount++;
				}
			}
		}

	} else {
		perror(NULL);
		assert(false);
	}

	assert(unrolledLinkedListSize(&list) == wordCount);

	rewind(file);
	struct UnrolledLinkedListIterator iterator;
	unrolledLinkedListInitializeIterator(&list, &iterator);
	while (!feof(file)) {
		if (NULL != fgets(buffer, BUFFER_SIZE, file)) {
			size_t length = strlen(buffer);
			if (length > 0) {
				if (buffer[length - 1] == '\n') {
					buffer[length - 1] = '\0';
					length--;
				}
			}

			if (0 <= length && length < KEY_LENGTH) {
				assert(iteratorHasNext(&iterator.iterator));
				struct Element* element = iteratorNext(&iterator.iterator);
				assert(strcmp(buffer, element->value) == 0);
			}
		}
	}

	fclose(file);

	unrolledLinkedListClear(&list);
}

int main(int argc, char** argv) {
	test1();
	test2();

	return EXIT_SUCCESS;
}
