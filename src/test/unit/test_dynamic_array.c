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
#include <stdlib.h>
#include <string.h>

#include "user/util/dynamic_array.h"

static void memoryAllocatorRelease(void* context, void* pointer) {
	free(pointer);
}

static void* memoryAllocatorResize(void* context, void* pointer, size_t size) {
	return realloc(pointer, size);
}

void test1(void) {
	struct DynamicArray dynamicArray;
	dynamicArrayInitialize(&dynamicArray, sizeof(char), NULL, &memoryAllocatorRelease, &memoryAllocatorResize);

	char* newElementPointer;
	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "A");
	assert(newElementPointer != NULL && *newElementPointer == 'A');

	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "B");
	assert(newElementPointer != NULL && *newElementPointer == 'B');

	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "C");
	assert(newElementPointer != NULL && *newElementPointer == 'C');

	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "D");
	assert(newElementPointer != NULL && *newElementPointer == 'D');
	assert(dynamicArraySize(&dynamicArray) == 4);

	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "E");
	assert(newElementPointer != NULL && *newElementPointer == 'E');
	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "F");
	assert(newElementPointer != NULL && *newElementPointer == 'F');
	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "G");
	assert(newElementPointer != NULL && *newElementPointer == 'G');
	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "H");
	assert(newElementPointer != NULL && *newElementPointer == 'H');
	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "I");
	assert(newElementPointer != NULL && *newElementPointer == 'I');
	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "J");
	assert(newElementPointer != NULL && *newElementPointer == 'J');
	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "K");
	assert(newElementPointer != NULL && *newElementPointer == 'K');
	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "L");
	assert(newElementPointer != NULL && *newElementPointer == 'L');
	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "M");
	assert(newElementPointer != NULL && *newElementPointer == 'M');
	newElementPointer = dynamicArrayInsertAfterLast(&dynamicArray, "N");
	assert(newElementPointer != NULL && *newElementPointer == 'N');
	assert(dynamicArraySize(&dynamicArray) == 14);

	char* string = malloc((dynamicArraySize(&dynamicArray) + 1) * sizeof(char));
	dynamicArrayCopy(&dynamicArray, string);
	string[dynamicArraySize(&dynamicArray)] = '\0';
	assert(strcmp(string, "ABCDEFGHIJKLMN") == 0);
	free(string);

	dynamicArrayClear(&dynamicArray, true);
	assert(dynamicArraySize(&dynamicArray) == 0);
}

void test2(void) {
	struct DynamicArray dynamicArray;
	dynamicArrayInitialize(&dynamicArray, sizeof(char), NULL, &memoryAllocatorRelease, &memoryAllocatorResize);

	const char* inputString = "glIlWu27MpZIxI432123AsDN8b38mQjgY2onFKlRwb1bElwPDMWdsRciLzcL";
	for (int i = 0; i < strlen(inputString); i++) {
		dynamicArrayInsertAfterLast(&dynamicArray, &inputString[i]);
	}
	assert(dynamicArraySize(&dynamicArray) == strlen(inputString));

	char* outputString = malloc((dynamicArraySize(&dynamicArray) + 1) * sizeof(char));
	dynamicArrayCopy(&dynamicArray, outputString);
	outputString[dynamicArraySize(&dynamicArray)] = '\0';
	assert(strcmp(outputString, inputString) == 0);
	free(outputString);

	dynamicArrayClear(&dynamicArray, true);
	assert(dynamicArraySize(&dynamicArray) == 0);
}

void test3(void) {
	struct DynamicArray dynamicArray;
	dynamicArrayInitialize(&dynamicArray, sizeof(int), NULL, &memoryAllocatorRelease, &memoryAllocatorResize);

	const int input[] = {
		380,
		4134,
		9285,
		3512,
		5758,
		750,
		4190,
		1238,
		4958,
		9305,
		5228,
		333,
		4689,
		4076,
		5995,
		3731,
		1773,
		1310,
		1546,
		8945,
		9374,
		5881,
		7838,
		4582,
		6536
	};
	for (int i = 0; i < sizeof(input) / sizeof(int); i++) {
		dynamicArrayInsertAfterLast(&dynamicArray, &input[i]);
	}
	assert(dynamicArraySize(&dynamicArray) == sizeof(input) / sizeof(int));

	int* output = malloc(dynamicArraySize(&dynamicArray) * sizeof(int));
	dynamicArrayCopy(&dynamicArray, output);
	for (int i = 0; i < sizeof(input) / sizeof(int); i++) {
		assert(input[i] == output[i]);
	}
	free(output);

	dynamicArrayClear(&dynamicArray, true);
	assert(dynamicArraySize(&dynamicArray) == 0);
}

int main(int argc, char** argv) {
	test1();
	test2();
	test3();

	return 0;
}
