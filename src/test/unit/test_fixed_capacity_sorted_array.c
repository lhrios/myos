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

#include "util/fixed_capacity_sorted_array.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Record {
	int key;
	char content[32];
};

static int recordKeyCompare(const int* record1Key, const int* record2Key) {
	return *record1Key - *record2Key;
}

static const int* recordKeyExtractor(const struct Record* record) {
	return &record->key;
}

static void test1(void) {
	#undef CAPACITY
	#define CAPACITY 20
	void* array = malloc(sizeof(struct Record) * CAPACITY);

	struct FixedCapacitySortedArray fixedCapacitySortedArray;
	fixedCapacitySortedArrayInitialize(&fixedCapacitySortedArray, sizeof(struct Record), array, sizeof(struct Record) * CAPACITY,
		(int (*)(const void*, const void*)) &recordKeyCompare,
		(const void* (*)(const void*)) &recordKeyExtractor
	);

	/* Insertion phase: */
	struct Record record;
	record.key = 4;
	sprintf(record.content, "%05d", -record.key);
	fixedCapacitySortedArrayInsert(&fixedCapacitySortedArray, &record);

	record.key = 1;
	sprintf(record.content, "%05d", -record.key);
	fixedCapacitySortedArrayInsert(&fixedCapacitySortedArray, &record);

	record.key = 15;
	sprintf(record.content, "%05d", -record.key);
	fixedCapacitySortedArrayInsert(&fixedCapacitySortedArray, &record);

	assert(fixedCapacitySortedArraySize(&fixedCapacitySortedArray) == 3);
	assert(fixedCapacitySortedArrayRemaining(&fixedCapacitySortedArray) == CAPACITY - 3);

	struct Record* result;
	int elementKey;

	elementKey = 2;
	result = fixedCapacitySortedArraySearch(&fixedCapacitySortedArray, &elementKey);
	assert(result == NULL);

	elementKey = 4;
	result = fixedCapacitySortedArraySearch(&fixedCapacitySortedArray, &elementKey);
	assert(result->key == 4);
	assert(strcmp("-0004", result->content) == 0);

	elementKey = 15;
	result = fixedCapacitySortedArraySearch(&fixedCapacitySortedArray, &elementKey);
	assert(result->key == 15);
	assert(strcmp("-0015", result->content) == 0);

	/* Remotion phase: */
	elementKey = 5;
	assert(false == fixedCapacitySortedArrayRemove(&fixedCapacitySortedArray, &elementKey));

	elementKey = 4;
	assert(true == fixedCapacitySortedArrayRemove(&fixedCapacitySortedArray, &elementKey));

	assert(fixedCapacitySortedArraySize(&fixedCapacitySortedArray) == 2);
	assert(fixedCapacitySortedArrayRemaining(&fixedCapacitySortedArray) == CAPACITY - 2);

	/* Access phase: */
	result = fixedCapacitySortedArrayGet(&fixedCapacitySortedArray, 0);
	assert(result->key == 1);
	assert(strcmp("-0001", result->content) == 0);

	result = fixedCapacitySortedArrayGet(&fixedCapacitySortedArray, 1);
	assert(result->key == 15);
	assert(strcmp("-0015", result->content) == 0);

	/* Search phase: */
	elementKey = 4;
	result = fixedCapacitySortedArraySearch(&fixedCapacitySortedArray, &elementKey);
	assert(result == NULL);

	elementKey = 15;
	result = fixedCapacitySortedArraySearch(&fixedCapacitySortedArray, &elementKey);
	assert(result->key == 15);
	assert(strcmp("-0015", result->content) == 0);

	elementKey = 1;
	result = fixedCapacitySortedArraySearch(&fixedCapacitySortedArray, &elementKey);
	assert(result->key == 1);
	assert(strcmp("-0001", result->content) == 0);

	free (array);
}

struct Document {
	int key;
	char content[64];
};

static int documentKeyCompare(const int* document1Key, const int* document2Key) {
	return *document1Key - *document2Key;
}

static const int* documentKeyExtractor(const struct Document** document) {
	return &(*document)->key;
}

static void test2(void) {
	#undef TOTAL_DOCUMENTS_COUNT
	#define TOTAL_DOCUMENTS_COUNT 4000
	struct Document** documents = malloc(sizeof(struct Document*) * TOTAL_DOCUMENTS_COUNT);
	memset(documents, 0, sizeof(struct Document*) * TOTAL_DOCUMENTS_COUNT);

	struct FixedCapacitySortedArray fixedCapacitySortedArray;
	struct Document* elements = malloc(sizeof(struct Document*) * TOTAL_DOCUMENTS_COUNT);
	fixedCapacitySortedArrayInitialize(&fixedCapacitySortedArray, sizeof(struct Document*), elements, sizeof(struct Document*) * TOTAL_DOCUMENTS_COUNT,
		(int (*)(const void*, const void*)) &documentKeyCompare,
		(const void* (*)(const void*)) &documentKeyExtractor
	);
	assert(fixedCapacitySortedArrayRemaining(&fixedCapacitySortedArray) == TOTAL_DOCUMENTS_COUNT);

	/* Insertion phase: */
	int documentCount = 0;
	while (documentCount < TOTAL_DOCUMENTS_COUNT) {
		struct Document* document = malloc(sizeof(struct Document));
		int documentKey = documentCount++;
		document->key = documentKey;
		sprintf(document->content, "%05d_%05d_%05d", documentKey, documentKey, documentKey);
		assert(fixedCapacitySortedArrayInsert(&fixedCapacitySortedArray, &document));
		documents[documentKey] = document;
	}
	assert(fixedCapacitySortedArrayRemaining(&fixedCapacitySortedArray) == 0);
	assert(fixedCapacitySortedArraySize(&fixedCapacitySortedArray) == TOTAL_DOCUMENTS_COUNT);

	/* Access phase: */
	for (int documentKey = 0; documentKey < documentCount; documentKey++) {
		struct Document** document = fixedCapacitySortedArrayGet(&fixedCapacitySortedArray, documentKey);
		assert(*document == documents[documentKey]);
		assert((*document)->key == documentKey);
		char content[64];
		sprintf(content, "%05d_%05d_%05d", documentKey, documentKey, documentKey);
		assert(strcmp(content, (*document)->content) == 0);
	}

	/* Search and remotion phase: */

   srand(1);

	while (documentCount > 0) {
		int documentKey = rand() % TOTAL_DOCUMENTS_COUNT;
		struct Document** document = fixedCapacitySortedArraySearch(&fixedCapacitySortedArray, &documentKey);
		if (documents[documentKey] != NULL) {
			assert(document != NULL);
			assert(*document == documents[documentKey]);
			assert((*document)->key == documentKey);
			char content[64];
			sprintf(content, "%05d_%05d_%05d", documentKey, documentKey, documentKey);
			assert(strcmp(content, (*document)->content) == 0);

			assert(fixedCapacitySortedArrayRemove(&fixedCapacitySortedArray, &documentKey));
			free(documents[documentKey]);
			documents[documentKey] = NULL;
			documentCount--;

		} else {
			assert(document == NULL);
		}
	}

	free(documents);
	free(elements);
}

static void test3(void) {
	#undef CAPACITY
	#define CAPACITY 100
	void* array = malloc(sizeof(struct Record) * CAPACITY);

	struct FixedCapacitySortedArray fixedCapacitySortedArray;
	fixedCapacitySortedArrayInitialize(&fixedCapacitySortedArray, sizeof(struct Record), array, sizeof(struct Record) * CAPACITY,
		(int (*)(const void*, const void*)) &recordKeyCompare,
		(const void* (*)(const void*)) &recordKeyExtractor
	);

	struct Record* records = malloc(sizeof(struct Record) * CAPACITY);
	struct Record* record;

	for (int i = 0; i < CAPACITY; i++) {
		record = &records[i];
		record->key = i;
		sprintf(record->content, "%05d", i);
		fixedCapacitySortedArrayInsert(&fixedCapacitySortedArray, record);
	}

	struct FixedCapacitySortedArrayIterator fixedCapacitySortedArrayIterator;
	fixedCapacitySortedArrayInitializeIterator(&fixedCapacitySortedArray, &fixedCapacitySortedArrayIterator);
	struct Iterator* iterator = &fixedCapacitySortedArrayIterator.iterator;

	int index = 0;
	while (iteratorHasNext(iterator)) {
		struct Record* retrievedRecord = iteratorNext(iterator);
		record = &records[index];
		assert(record->key == index);
		assert(strcmp(record->content, retrievedRecord->content) == 0);
		assert(atoi(record->content) == index);
		index++;
	}
	assert(index == CAPACITY);

	free (records);
	free (array);
}

int main(int argc, char** argv) {
	test1();
	test2();
	test3();

	return 0;
}
