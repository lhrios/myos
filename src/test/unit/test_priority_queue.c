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

#include "util/priority_queue.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool integerComparator1(const void* a, const void* b, void* argument1, void* argument2, void* argument3) {
	int32_t aAsInteger = (*(int32_t*) a);
	int32_t bAsInteger = (*(int32_t*) b);
	return aAsInteger < bAsInteger;
}

static void test1(void) {
	struct PriorityQueue priorityQueue;
	int capacity = 10;
	int32_t* queue = malloc(sizeof(int32_t) * capacity);
	priorityQueueInitialize(&priorityQueue, queue, capacity, 0, sizeof(int32_t), &integerComparator1, NULL, NULL, NULL);
	assert(0 == priorityQueueSize(&priorityQueue));

	bool insertionResult;
	int32_t item;

	item = 13;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);
	assert(1 == priorityQueueSize(&priorityQueue));

	item = 0;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);
	assert(2 == priorityQueueSize(&priorityQueue));

	item = 5;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);
	assert(3 == priorityQueueSize(&priorityQueue));

	item = 15;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);
	assert(4 == priorityQueueSize(&priorityQueue));

	item = 25;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);
	assert(5 == priorityQueueSize(&priorityQueue));

	item = 200;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);
	assert(6 == priorityQueueSize(&priorityQueue));

	item = 17;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);
	assert(7 == priorityQueueSize(&priorityQueue));

	item = -1;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);
	assert(8 == priorityQueueSize(&priorityQueue));

	item = 96;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);
	assert(9 == priorityQueueSize(&priorityQueue));

	item = 31;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);
	assert(10 == priorityQueueSize(&priorityQueue));

	item = 54;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(!insertionResult);
	assert(10 == priorityQueueSize(&priorityQueue));

	priorityQueuePeek(&priorityQueue, &item);
	assert(item == -1);
	assert(10 == priorityQueueSize(&priorityQueue));
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == -1);
	assert(9 == priorityQueueSize(&priorityQueue));

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 0);
	assert(8 == priorityQueueSize(&priorityQueue));

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 5);
	assert(7 == priorityQueueSize(&priorityQueue));

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 13);
	assert(6 == priorityQueueSize(&priorityQueue));

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 15);
	assert(5 == priorityQueueSize(&priorityQueue));

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 17);
	assert(4 == priorityQueueSize(&priorityQueue));

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 25);
	assert(3 == priorityQueueSize(&priorityQueue));

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 31);
	assert(2 == priorityQueueSize(&priorityQueue));

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 96);
	assert(1 == priorityQueueSize(&priorityQueue));

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 200);
	assert(0 == priorityQueueSize(&priorityQueue));

	free(queue);
}

static void test2(void) {
	struct PriorityQueue priorityQueue;
	int capacity = 100;
	int32_t* queue = malloc(sizeof(int32_t) * capacity);
	priorityQueueInitialize(&priorityQueue, queue, capacity, 0, sizeof(int32_t), &integerComparator1, NULL, NULL, NULL);
	assert(0 == priorityQueueSize(&priorityQueue));

	bool insertionResult;
	int32_t item;

	item = 10;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);

	item = 9;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);

	item = 8;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);

	item = 7;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);

	item = 6;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);

	assert(5 == priorityQueueSize(&priorityQueue));

	priorityQueuePeek(&priorityQueue, &item);
	assert(item == 6);
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 6);

	item = 1;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);

	item = 2;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);

	item = 3;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);

	item = 4;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);

	assert(8 == priorityQueueSize(&priorityQueue));

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 1);

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 2);

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 3);

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 4);

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 7);

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 8);

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 9);

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 10);

	assert(0 == priorityQueueSize(&priorityQueue));

	free(queue);
}

static void test3(void) {
	struct PriorityQueue priorityQueue;
	int capacity = 10;
	int32_t* queue = malloc(sizeof(int32_t) * capacity);
	queue[0] = 10;
	queue[1] = 1;
	queue[2] = 6;
	queue[3] = 7;
	queue[4] = 4;
	queue[5] = 3;
	queue[6] = 2;
	queue[7] = 5;
	queue[8] = 8;
	queue[9] = 9;
	priorityQueueInitialize(&priorityQueue, queue, capacity, capacity, sizeof(int32_t), &integerComparator1, NULL, NULL, NULL);
	assert(capacity == priorityQueueSize(&priorityQueue));

	int32_t item = 0;

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 1);
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 2);

	bool insertionResult;

	item = 0;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);

	item = 11;
	insertionResult = priorityQueueInsert(&priorityQueue, &item);
	assert(insertionResult);

	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 0);
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 3);
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 4);
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 5);
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 6);
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 7);
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 8);
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 9);
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 10);
	priorityQueueRemove(&priorityQueue, &item);
	assert(item == 11);

	assert(0 == priorityQueueSize(&priorityQueue));

	free(queue);
}

static int integerComparator2(const void* item1, const void* item2) {
	return *((int*) item1) - *((int*) item2);
}

static void test4(void) {
	const size_t arrayLength = 15;
	int array[arrayLength];
	array[0] = 48;
	array[1] = 1;
	array[2] = 5;
	array[3] = 89;
	array[4] = 23;
	array[5] = 52;
	array[6] = 100;
	array[7] = -500;
	array[8] = 38;
	array[9] = 49;
	array[10] = -1000;
	array[11] = 15;
	array[12] = -15;
	array[13] = 0;
	array[14] = 0;

	priorityQueueInplaceArraySort(array, arrayLength, sizeof(int), (int (*)(const void*, const void*)) &integerComparator2, false, NULL);

	assert(array[0] == -1000);
	assert(array[1] == -500);
	assert(array[2] == -15);
	assert(array[3] == 0);
	assert(array[4] == 0);
	assert(array[5] == 1);
	assert(array[6] == 5);
	assert(array[7] == 15);
	assert(array[8] == 23);
	assert(array[9] == 38);
	assert(array[10] == 48);
	assert(array[11] == 49);
	assert(array[12] == 52);
	assert(array[13] == 89);
	assert(array[14] == 100);
}

static int stringComparator(const void* item1, const void* item2) {
	return strcmp(*((const char**) item1), *((const char**) item2));
}

static void test5(void) {
	const int BUFFER_SIZE = 256;
	char buffer[BUFFER_SIZE];

	const int MAX_WORDS = 5000;
	char** words = malloc(sizeof(char*) * MAX_WORDS);

	int wordCount = 0;
	FILE* file = fopen("../resources/test/unit/words.txt", "r");
	if (file != NULL) {
		while (!feof(file) && wordCount < MAX_WORDS) {
			if (NULL != fgets(buffer, BUFFER_SIZE, file)) {
				size_t length = strlen(buffer);
				if (length > 0) {
					if (buffer[length - 1] == '\n') {
						buffer[length - 1] = '\0';
						length--;
					}
				}

				words[wordCount++] = strdup(buffer);
			}
		}
		fclose(file);

	} else {
		perror(NULL);
		assert(false);
	}

	priorityQueueInplaceArraySort(words, wordCount, sizeof(char*), (int (*)(const void*, const void*)) &stringComparator, false, NULL);
	for (int i = 1; i < wordCount; i++) {
		assert(strcmp(words[i - 1], words[i]) <= 0);
	}

	for (int i = 0; i < wordCount; i++) {
		free(words[i]);
	}
	free(words);
}

int main(int argc, char** argv) {
	test1();
	test2();
	test3();
	test4();
	test5();

	return 0;
}
