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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util/double_linked_list.h"

#include "test/integration_test.h"

struct Element {
	struct DoubleLinkedListElement doubleLinkedListElement;
	char* copies;
};

#define NUMBER_OF_COPIES 128
struct Element* createElement(const char* content) {
	size_t contentLength = strlen(content);
	char* copies = malloc(sizeof(char) * NUMBER_OF_COPIES * contentLength);
	assert(copies);
	for (int copyId = 0; copyId < NUMBER_OF_COPIES; copyId++) {
		strncpy(copies + copyId * contentLength, content, contentLength);
	}

	struct Element* element = malloc(sizeof(struct Element));
	assert(element != NULL);
	element->copies = copies;

	return element;
}

bool isElementValid(struct Element* element, const char* content) {
	size_t contentLength = strlen(content);
	for (int copyId = 0; copyId < NUMBER_OF_COPIES; copyId++) {
		if (strncmp(element->copies + copyId * contentLength, content, contentLength) != 0) {
			return false;
		}
	}

	return true;
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	struct DoubleLinkedList list;
	doubleLinkedListInitialize(&list);

	FILE* file = fopen("/resources/words.txt", "r");
	assert(file != NULL);

	const size_t bufferSize = 1024 * sizeof(char);
	char* buffer = malloc(bufferSize);
	assert(buffer != NULL);

	while (true) {
		char* key = fgets(buffer, bufferSize, file);
		if (key == NULL) {
			break;
		} else {
			struct Element* element = createElement(buffer);
			doubleLinkedListInsertAfterLast(&list, &element->doubleLinkedListElement);
		}
	}
	assert(feof(file));

	{
		void* result = sbrk(1024 * 1024 * 128);
		assert(result != (void*)-1);

		sleep(15);

		result = sbrk(1024 * 1024 * -128);
		assert(result != (void*)-1);
	}

	rewind(file);

	struct DoubleLinkedListElement* nextElement = doubleLinkedListFirst(&list);
	while (true) {
		char* key = fgets(buffer, bufferSize, file);
		if (key == NULL) {
			assert(nextElement == NULL);
			break;

		} else {
			assert(nextElement != NULL);
			struct Element* element = (void*) nextElement;
			assert(isElementValid(element, buffer));
			nextElement = nextElement->next;
		}
	}
	assert(feof(file));

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
