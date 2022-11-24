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
#include <string.h>

#include "util/search_utils.h"

static int compare(const char** value1, const char** value2) {
	return strcmp(*value1, *value2);
}

static void test1(void) {
	const char* array[] = {
		"b", /* 0 */
		"c", /* 1 */
		"e", /* 2 */
		"f", /* 3 */
		"p", /* 4 */
		"y" /* 5 */
	};

	size_t arraySize = 6;
	const char* key;
	int result;

	key = "q";
	result = searchUtilsBinarySearchComparingElements((void*) &key, array, arraySize, sizeof(void*), (int(*)(const void*, const void*)) &compare);
	assert(result == -6);

	key = "c";
	result = searchUtilsBinarySearchComparingElements((void*) &key, array, arraySize, sizeof(void*), (int(*)(const void*, const void*)) &compare);
	assert(result == 1);

	key = "z";
	result = searchUtilsBinarySearchComparingElements((void*) &key, array, arraySize, sizeof(void*), (int(*)(const void*, const void*)) &compare);
	assert(result == -7);

	key = "a";
	result = searchUtilsBinarySearchComparingElements((void*) &key, array, arraySize, sizeof(void*), (int(*)(const void*, const void*)) &compare);
	assert(result == -1);

	key = "d";
	result = searchUtilsBinarySearchComparingElements((void*) &key, array, arraySize, sizeof(void*), (int(*)(const void*, const void*)) &compare);
	assert(result == -3);
}

struct Record {
	int key;
	char value[8];
};

static int recordKeyCompare(const int* record1Key, const int* record2Key) {
	return *record1Key - *record2Key;
}

static const int* recordKeyExtractor(const struct Record** record) {
	return &(*record)->key;
}

static void test2(void) {
	struct Record record0;
	record0.key = 0;
	strcpy(record0.value, "zero");

	struct Record record1;
	record1.key = 1;
	strcpy(record1.value, "one");

	struct Record record2;
	record2.key = 2;
	strcpy(record2.value, "two");

	struct Record record3;
	record3.key = 3;
	strcpy(record3.value, "three");

	struct Record record4;
	record4.key = 4;
	strcpy(record4.value, "four");

	struct Record record5;
	record5.key = 5;
	strcpy(record5.value, "five");

	struct Record record6;
	record6.key = 6;
	strcpy(record6.value, "six");

	struct Record record7;
	record7.key = 7;
	strcpy(record7.value, "seven");

	struct Record record8;
	record8.key = 8;
	strcpy(record8.value, "eight");
	
	const struct Record* elements[] = {
		&record0,
		&record1,
		&record2,
		&record3,
		&record4,
		&record5,
		&record6,
		&record7,
		&record8
	};

	const struct Record* record;
	int key;
	int index;

	key = 0;
	index = searchUtilsBinarySearchComparingKeys(&key, elements, sizeof(elements) / sizeof(struct Record*), sizeof(void*),
			(int(*)(const void*, const void*)) &recordKeyCompare, (const void* (*)(const void*)) &recordKeyExtractor);
	assert(index == 0);
	record = elements[index];
	assert(strcmp("zero", record->value) == 0);

	key = 6;
	index = searchUtilsBinarySearchComparingKeys(&key, elements, sizeof(elements) / sizeof(struct Record*), sizeof(void*),
			(int(*)(const void*, const void*)) &recordKeyCompare, (const void* (*)(const void*)) &recordKeyExtractor);
	assert(index == 6);
	record = elements[index];
	assert(strcmp("six", record->value) == 0);

	key = 9;
	index = searchUtilsBinarySearchComparingKeys(&key, elements, sizeof(elements) / sizeof(struct Record*), sizeof(void*),
			(int(*)(const void*, const void*)) &recordKeyCompare, (const void* (*)(const void*)) &recordKeyExtractor);
	assert(index == -10);

	key = -1;
	index = searchUtilsBinarySearchComparingKeys(&key, elements, sizeof(elements) / sizeof(struct Record*), sizeof(void*),
			(int(*)(const void*, const void*)) &recordKeyCompare, (const void* (*)(const void*)) &recordKeyExtractor);
	assert(index == -1);

	key = 8;
	index = searchUtilsBinarySearchComparingKeys(&key, elements, sizeof(elements) / sizeof(struct Record*), sizeof(void*),
			(int(*)(const void*, const void*)) &recordKeyCompare, (const void* (*)(const void*)) &recordKeyExtractor);
	assert(index == 8);
	record = elements[index];
	assert(strcmp("eight", record->value) == 0);
}

int main(int argc, char** argv) {
	test1();
	test2();

	return 0;
}
