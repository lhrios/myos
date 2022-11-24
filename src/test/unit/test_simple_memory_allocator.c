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
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "user/util/simple_memory_allocator.h"

#include "util/math_utils.h"

#define DATA_SEGMENT_SIZE (512 * 1024 * 1024)
void* dataSegmentBegin;
void* currentDataSegmentEnd;
void* dataSegmentEnd;

static void* fakeSbrk(int increment) {
	if (increment == 0) {
		return currentDataSegmentEnd;
	} else if (currentDataSegmentEnd + increment <= dataSegmentEnd) {
		currentDataSegmentEnd += increment;
		return currentDataSegmentEnd;
	} else {
		return (void*) -1;
	}
}

static bool isContentValid(int memoryAcquisitionId, int* array, int length) {
	for (int i = 0; i < length; i++) {
		if (array[i] != memoryAcquisitionId) {
			return false;
		}
	}
	return true;
}

static void writeContent(int memoryAcquisitionId, int* array, int length) {
	for (int i = 0; i < length; i++) {
		array[i] = memoryAcquisitionId;
	}
}

#define MEMORY_ACQUIRE_TOTAL_COUNT 5000
#define INTEGER_ARRAY_MAX_LENGTH 15000
static void test1(void) {
	int lengths[MEMORY_ACQUIRE_TOTAL_COUNT];
	int* pointers[MEMORY_ACQUIRE_TOTAL_COUNT];
	
	srand(0);
	//srand(time(NULL));
	
	dataSegmentBegin = malloc(DATA_SEGMENT_SIZE);
	currentDataSegmentEnd = dataSegmentBegin;
	dataSegmentEnd = dataSegmentBegin +  DATA_SEGMENT_SIZE;

	simpleMemoryAllocatorInitialize(&fakeSbrk);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	
	for (int memoryAcquireCount = 0; memoryAcquireCount < MEMORY_ACQUIRE_TOTAL_COUNT; memoryAcquireCount++) {		
		if (memoryAcquireCount > 0 && (((float) rand()) / RAND_MAX) <= 0.25) {
			int index = rand() % memoryAcquireCount;
			if (pointers[index] != NULL) {
				assert(isContentValid(index, pointers[index], lengths[index]));				 
				simpleMemoryAllocatorRelease(pointers[index]);
				assert(simpleMemoryAllocatorIsInternalStateValid());
				pointers[index] = NULL;
			}
		}
	
		int length = (rand() % INTEGER_ARRAY_MAX_LENGTH) + 1;
		lengths[memoryAcquireCount] = length;
		assert(1 <= length && length <= INTEGER_ARRAY_MAX_LENGTH);
		int* array = simpleMemoryAllocatorAcquire(length * sizeof(int));
		assert(array != NULL);
		assert(simpleMemoryAllocatorIsInternalStateValid());
		pointers[memoryAcquireCount] = array;
		
		writeContent(memoryAcquireCount, array, length);
	}

	for (int i = 0; i < MEMORY_ACQUIRE_TOTAL_COUNT; i++) {
		int* array = pointers[i];

		if (array != NULL) {
			float probability = ((float) rand()) / RAND_MAX;

			if (probability <= 0.50) {
				int length = lengths[i];
				length /= 2;
				lengths[i] = length;

				int* newArray = simpleMemoryAllocatorResize(array, length * sizeof(int));
				assert(newArray == array);
				assert(simpleMemoryAllocatorIsInternalStateValid());

			} else if (probability <= 0.75) {
				int length = lengths[i];
				length = mathUtilsMax(length * 2, INTEGER_ARRAY_MAX_LENGTH);
				lengths[i] = length;

				int* newArray = simpleMemoryAllocatorResize(array, length * sizeof(int));
				assert(newArray != NULL);
				assert(simpleMemoryAllocatorIsInternalStateValid());
				writeContent(i, newArray, length);
				pointers[i] = newArray;
			}
		}
	}
	
	for (int i = 0; i < MEMORY_ACQUIRE_TOTAL_COUNT; i++) {	  
		if (pointers[i] != NULL) {
			assert(isContentValid(i, pointers[i], lengths[i]));
			simpleMemoryAllocatorRelease(pointers[i]);
			assert(simpleMemoryAllocatorIsInternalStateValid());
			pointers[i] = NULL;
		}
	}

	free(dataSegmentBegin);
}

static void test2(void) {
	dataSegmentBegin = malloc(DATA_SEGMENT_SIZE);
	currentDataSegmentEnd = dataSegmentBegin;
	dataSegmentEnd = dataSegmentBegin +  DATA_SEGMENT_SIZE;
	
	simpleMemoryAllocatorInitialize(&fakeSbrk);
	void* pointer1 = simpleMemoryAllocatorAcquire(105);
	assert(pointer1 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	
	void* pointer2 = simpleMemoryAllocatorAcquire(1105);
	assert(pointer2 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	
	void* pointer3 = simpleMemoryAllocatorAcquire(1048596);
	assert(pointer3 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	
	void* pointer4 = simpleMemoryAllocatorAcquire(1);
	assert(pointer4 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	 
	simpleMemoryAllocatorRelease(pointer1);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	 
	simpleMemoryAllocatorRelease(pointer3);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	
	simpleMemoryAllocatorRelease(pointer2);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	
	void* pointer5 = simpleMemoryAllocatorAcquire(1);
	assert(pointer5 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());

	free(dataSegmentBegin);
}

static void test3(void) {
	dataSegmentBegin = malloc(DATA_SEGMENT_SIZE);
	currentDataSegmentEnd = dataSegmentBegin;
	dataSegmentEnd = dataSegmentBegin +  DATA_SEGMENT_SIZE;

	simpleMemoryAllocatorInitialize(&fakeSbrk);
	assert(simpleMemoryAllocatorIsInternalStateValid());

	size_t lengthPointer1 = 105;
	void* pointer1 = simpleMemoryAllocatorAcquire(lengthPointer1 * sizeof(int));
	assert(pointer1 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(1, pointer1, lengthPointer1);

	size_t lengthPointer2 = 1377;
	void* pointer2 = simpleMemoryAllocatorAcquire(lengthPointer2 * sizeof(int));
	assert(pointer2 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(2, pointer2, lengthPointer2);

	size_t lengthPointer4 = 5000;
	void* pointer4 = simpleMemoryAllocatorAcquire(lengthPointer4 * sizeof(int));
	assert(pointer4 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(4, pointer4, lengthPointer4);

	assert(isContentValid(1, pointer1, lengthPointer1));
	assert(isContentValid(2, pointer2, lengthPointer2));
	assert(isContentValid(4, pointer4, lengthPointer4));

	lengthPointer2 = 1000;
	void* newPointer2 = simpleMemoryAllocatorResize(pointer2, lengthPointer2 * sizeof(int));
	assert(newPointer2 == pointer2);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	assert(isContentValid(1, pointer1, lengthPointer1));
	assert(isContentValid(2, pointer2, lengthPointer2));
	assert(isContentValid(4, pointer4, lengthPointer4));

	lengthPointer1 = 100;
	void* newPointer1 = simpleMemoryAllocatorResize(pointer1, lengthPointer1 * sizeof(int));
	assert(newPointer1 == pointer1);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	assert(isContentValid(1, pointer1, lengthPointer1));
	assert(isContentValid(2, pointer2, lengthPointer2));
	assert(isContentValid(4, pointer4, lengthPointer4));

	size_t lengthPointer3 = 10;
	void* pointer3 = simpleMemoryAllocatorAcquire(lengthPointer3 * sizeof(int));
	assert(pointer3 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(3, pointer3, lengthPointer3);

	size_t lengthPointer5 = 5000;
	void* pointer5 = simpleMemoryAllocatorAcquire(lengthPointer5 * sizeof(int));
	assert(pointer5 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(5, pointer5, lengthPointer5);

	assert(isContentValid(1, pointer1, lengthPointer1));
	assert(isContentValid(2, pointer2, lengthPointer2));
	assert(isContentValid(3, pointer3, lengthPointer3));
	assert(isContentValid(4, pointer4, lengthPointer4));
	assert(isContentValid(5, pointer5, lengthPointer5));

	simpleMemoryAllocatorRelease(pointer3);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	assert(isContentValid(1, pointer1, lengthPointer1));
	assert(isContentValid(2, pointer2, lengthPointer2));
	assert(isContentValid(4, pointer4, lengthPointer4));
	assert(isContentValid(5, pointer5, lengthPointer5));

	simpleMemoryAllocatorRelease(pointer2);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	assert(isContentValid(1, pointer1, lengthPointer1));
	assert(isContentValid(4, pointer4, lengthPointer4));
	assert(isContentValid(5, pointer5, lengthPointer5));

	simpleMemoryAllocatorRelease(pointer4);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	simpleMemoryAllocatorRelease(pointer1);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	simpleMemoryAllocatorRelease(pointer5);
	assert(simpleMemoryAllocatorIsInternalStateValid());

	free(dataSegmentBegin);
}

static void test4(void) {
	dataSegmentBegin = malloc(DATA_SEGMENT_SIZE);
	currentDataSegmentEnd = dataSegmentBegin;
	dataSegmentEnd = dataSegmentBegin +  DATA_SEGMENT_SIZE;

	simpleMemoryAllocatorInitialize(&fakeSbrk);

	size_t lengthPointer1 = 105;
	void* pointer1 = simpleMemoryAllocatorAcquire(lengthPointer1 * sizeof(int));
	assert(pointer1 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(1, pointer1, lengthPointer1);

	size_t lengthPointer2 = 1377;
	void* pointer2 = simpleMemoryAllocatorAcquire(lengthPointer2 * sizeof(int));
	assert(pointer2 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(2, pointer2, lengthPointer2);

	size_t lengthPointer3 = 14540;
	void* pointer3 = simpleMemoryAllocatorAcquire(lengthPointer3 * sizeof(int));
	assert(pointer3 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(3, pointer3, lengthPointer3);

	size_t lengthPointer4 = 1234;
	void* pointer4 = simpleMemoryAllocatorAcquire(lengthPointer4 * sizeof(int));
	assert(pointer4 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(4, pointer4, lengthPointer4);

	size_t lengthPointer5 = 500;
	void* pointer5 = simpleMemoryAllocatorAcquire(lengthPointer5 * sizeof(int));
	assert(pointer5 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(5, pointer5, lengthPointer5);

	assert(isContentValid(1, pointer1, lengthPointer1));
	assert(isContentValid(2, pointer2, lengthPointer2));
	assert(isContentValid(3, pointer3, lengthPointer3));
	assert(isContentValid(4, pointer4, lengthPointer4));
	assert(isContentValid(5, pointer5, lengthPointer5));

	simpleMemoryAllocatorRelease(pointer3);
	assert(simpleMemoryAllocatorIsInternalStateValid());

	lengthPointer2 = 300;
	void* newPointer2 = simpleMemoryAllocatorResize(pointer2, lengthPointer2 * sizeof(int));
	assert(newPointer2 == pointer2);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	assert(isContentValid(1, pointer1, lengthPointer1));
	assert(isContentValid(2, pointer2, lengthPointer2));
	assert(isContentValid(4, pointer4, lengthPointer4));
	assert(isContentValid(5, pointer5, lengthPointer5));

	simpleMemoryAllocatorRelease(pointer5);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	simpleMemoryAllocatorRelease(pointer1);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	simpleMemoryAllocatorRelease(pointer4);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	simpleMemoryAllocatorRelease(pointer2);
	assert(simpleMemoryAllocatorIsInternalStateValid());

	free(dataSegmentBegin);
}

static void test5(void) {
	dataSegmentBegin = malloc(DATA_SEGMENT_SIZE);
	currentDataSegmentEnd = dataSegmentBegin;
	dataSegmentEnd = dataSegmentBegin +  DATA_SEGMENT_SIZE;

	simpleMemoryAllocatorInitialize(&fakeSbrk);

	size_t lengthPointer1 = 105;
	void* pointer1 = simpleMemoryAllocatorAcquire(lengthPointer1 * sizeof(int));
	assert(pointer1 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(1, pointer1, lengthPointer1);

	size_t lengthPointer2 = 1377;
	void* pointer2 = simpleMemoryAllocatorAcquire(lengthPointer2 * sizeof(int));
	assert(pointer2 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(2, pointer2, lengthPointer2);

	size_t lengthPointer3 = 14540;
	void* pointer3 = simpleMemoryAllocatorAcquire(lengthPointer3 * sizeof(int));
	assert(pointer3 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(3, pointer3, lengthPointer3);

	size_t lengthPointer4 = 1234;
	void* pointer4 = simpleMemoryAllocatorAcquire(lengthPointer4 * sizeof(int));
	assert(pointer4 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(4, pointer4, lengthPointer4);

	size_t lengthPointer5 = 500;
	void* pointer5 = simpleMemoryAllocatorAcquire(lengthPointer5 * sizeof(int));
	assert(pointer5 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(5, pointer5, lengthPointer5);

	assert(isContentValid(1, pointer1, lengthPointer1));
	assert(isContentValid(2, pointer2, lengthPointer2));
	assert(isContentValid(3, pointer3, lengthPointer3));
	assert(isContentValid(4, pointer4, lengthPointer4));
	assert(isContentValid(5, pointer5, lengthPointer5));

	simpleMemoryAllocatorRelease(pointer2);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	simpleMemoryAllocatorRelease(pointer3);
	assert(simpleMemoryAllocatorIsInternalStateValid());

	lengthPointer1 += lengthPointer2 + lengthPointer3;
	void* newPointer1 = simpleMemoryAllocatorResize(pointer1, lengthPointer1 * sizeof(int));
	assert(newPointer1 == pointer1);

	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(1, pointer1, lengthPointer1);

	assert(isContentValid(4, pointer4, lengthPointer4));
	assert(isContentValid(5, pointer5, lengthPointer5));
	assert(isContentValid(1, pointer1, lengthPointer1));

	int newLengthPointer1 = lengthPointer1 * 3;
	newPointer1 = simpleMemoryAllocatorResize(pointer1, newLengthPointer1 * sizeof(int));
	assert(newPointer1 != pointer1);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	pointer1 = newPointer1;

	assert(isContentValid(4, pointer4, lengthPointer4));
	assert(isContentValid(5, pointer5, lengthPointer5));
	assert(isContentValid(1, pointer1, lengthPointer1));

	lengthPointer1 = newLengthPointer1;
	writeContent(1, pointer1, lengthPointer1);

	assert(isContentValid(4, pointer4, lengthPointer4));
	assert(isContentValid(5, pointer5, lengthPointer5));
	assert(isContentValid(1, pointer1, lengthPointer1));

	simpleMemoryAllocatorRelease(pointer5);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	simpleMemoryAllocatorRelease(pointer1);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	simpleMemoryAllocatorRelease(pointer4);
	assert(simpleMemoryAllocatorIsInternalStateValid());

	free(dataSegmentBegin);
}

static void test6(void) {
	dataSegmentBegin = malloc(DATA_SEGMENT_SIZE);
	currentDataSegmentEnd = dataSegmentBegin;
	dataSegmentEnd = dataSegmentBegin +  DATA_SEGMENT_SIZE;

	simpleMemoryAllocatorInitialize(&fakeSbrk);

	size_t lengthPointer1 = 105;
	void* pointer1 = simpleMemoryAllocatorAcquire(lengthPointer1 * sizeof(int));
	assert(pointer1 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(1, pointer1, lengthPointer1);

	size_t lengthPointer2 = 1377;
	void* pointer2 = simpleMemoryAllocatorAcquire(lengthPointer2 * sizeof(int));
	assert(pointer2 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(2, pointer2, lengthPointer2);

	size_t lengthPointer3 = 14540;
	void* pointer3 = simpleMemoryAllocatorAcquire(lengthPointer3 * sizeof(int));
	assert(pointer3 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(3, pointer3, lengthPointer3);

	size_t lengthPointer4 = 1234;
	void* pointer4 = simpleMemoryAllocatorAcquire(lengthPointer4 * sizeof(int));
	assert(pointer4 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(4, pointer4, lengthPointer4);

	size_t lengthPointer5 = 500;
	void* pointer5 = simpleMemoryAllocatorAcquire(lengthPointer5 * sizeof(int));
	assert(pointer5 != NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	writeContent(5, pointer5, lengthPointer5);

	assert(isContentValid(1, pointer1, lengthPointer1));
	assert(isContentValid(2, pointer2, lengthPointer2));
	assert(isContentValid(3, pointer3, lengthPointer3));
	assert(isContentValid(4, pointer4, lengthPointer4));
	assert(isContentValid(5, pointer5, lengthPointer5));

	lengthPointer1 = 0;
	void* newPointer1 = simpleMemoryAllocatorResize(pointer1, lengthPointer1 * sizeof(int));
	assert(newPointer1 == NULL);
	assert(simpleMemoryAllocatorIsInternalStateValid());

	assert(isContentValid(4, pointer4, lengthPointer4));
	assert(isContentValid(5, pointer5, lengthPointer5));
	assert(isContentValid(2, pointer2, lengthPointer2));
	assert(isContentValid(3, pointer3, lengthPointer3));

	simpleMemoryAllocatorRelease(pointer5);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	simpleMemoryAllocatorRelease(pointer4);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	simpleMemoryAllocatorRelease(pointer2);
	assert(simpleMemoryAllocatorIsInternalStateValid());
	simpleMemoryAllocatorRelease(pointer3);
	assert(simpleMemoryAllocatorIsInternalStateValid());

	free(dataSegmentBegin);
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
