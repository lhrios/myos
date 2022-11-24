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
#include <string.h>

#include "util/fixed_capacity_sorted_array.h"
#include "util/search_utils.h"

static void* getElementAddress(struct FixedCapacitySortedArray* fixedCapacitySortedArray, int index) {
	return fixedCapacitySortedArray->elements + (index * fixedCapacitySortedArray->elementSize);
}

void fixedCapacitySortedArrayInitialize(struct FixedCapacitySortedArray* fixedCapacitySortedArray, size_t elementSize,
		void* array, int arrayCapacityInBytes, int (*elementKeyComparator)(const void*, const void*), const void* (*elementKeyExtractor)(const void*)) {

	fixedCapacitySortedArray->size = 0;
	fixedCapacitySortedArray->elements = array;
	fixedCapacitySortedArray->elementSize = elementSize;
	fixedCapacitySortedArray->capacity = arrayCapacityInBytes / elementSize;
	fixedCapacitySortedArray->elementKeyComparator = elementKeyComparator;
	fixedCapacitySortedArray->elementKeyExtractor = elementKeyExtractor;
}

bool fixedCapacitySortedArrayInsert(struct FixedCapacitySortedArray* fixedCapacitySortedArray, const void* element) {
	int size = fixedCapacitySortedArray->size;
	int elementSize = fixedCapacitySortedArray->elementSize;

	if (size + 1 <= fixedCapacitySortedArray->capacity) {
		const void* elementKey = fixedCapacitySortedArray->elementKeyExtractor(element);
			int index = searchUtilsBinarySearchComparingKeys(elementKey, fixedCapacitySortedArray->elements, fixedCapacitySortedArray->size, elementSize,
				fixedCapacitySortedArray->elementKeyComparator, fixedCapacitySortedArray->elementKeyExtractor);

		if (index < 0) {
			index = -(index + 1);
			memmove(getElementAddress(fixedCapacitySortedArray, index + 1), getElementAddress(fixedCapacitySortedArray, index), elementSize * (size - index));
			memcpy(getElementAddress(fixedCapacitySortedArray, index), element, elementSize);
			fixedCapacitySortedArray->size++;
			return true;
		}
	}

	return false;
}

bool fixedCapacitySortedArrayRemove(struct FixedCapacitySortedArray* fixedCapacitySortedArray, const void* elementKey) {
	int size = fixedCapacitySortedArray->size;
	int elementSize = fixedCapacitySortedArray->elementSize;

	int index = searchUtilsBinarySearchComparingKeys(elementKey, fixedCapacitySortedArray->elements, fixedCapacitySortedArray->size, elementSize,
		fixedCapacitySortedArray->elementKeyComparator, fixedCapacitySortedArray->elementKeyExtractor);
	if (index >= 0) {
		memmove(getElementAddress(fixedCapacitySortedArray, index), getElementAddress(fixedCapacitySortedArray, index + 1), elementSize * (size - (index + 1)));
		assert(size > 0);
		fixedCapacitySortedArray->size--;
		return true;
	}

	return false;
}

void* fixedCapacitySortedArraySearch(struct FixedCapacitySortedArray* fixedCapacitySortedArray, const void* elementKey) {
	int index = searchUtilsBinarySearchComparingKeys(elementKey, fixedCapacitySortedArray->elements, fixedCapacitySortedArray->size,
		fixedCapacitySortedArray->elementSize, fixedCapacitySortedArray->elementKeyComparator, fixedCapacitySortedArray->elementKeyExtractor);
	if (index >= 0) {
		return getElementAddress(fixedCapacitySortedArray, index);
	} else {
		return NULL;
	}
}

void* fixedCapacitySortedArrayGet(struct FixedCapacitySortedArray* fixedCapacitySortedArray, int index) {
	assert(0 <= index && index < fixedCapacitySortedArray->size);
	return getElementAddress(fixedCapacitySortedArray, index);
}

static bool hasNext(struct Iterator* iterator) {
	struct FixedCapacitySortedArrayIterator* fixedCapacitySortedArrayIterator = (void*) iterator;
	struct FixedCapacitySortedArray* fixedCapacitySortedArray = fixedCapacitySortedArrayIterator->fixedCapacitySortedArray;
	return fixedCapacitySortedArraySize(fixedCapacitySortedArray) > fixedCapacitySortedArrayIterator->nextIndex;
}

static void* next(struct Iterator* iterator) {
	struct FixedCapacitySortedArrayIterator* fixedCapacitySortedArrayIterator = (void*) iterator;
	struct FixedCapacitySortedArray* fixedCapacitySortedArray = fixedCapacitySortedArrayIterator->fixedCapacitySortedArray;
	return fixedCapacitySortedArrayGet(fixedCapacitySortedArray, fixedCapacitySortedArrayIterator->nextIndex++);
}

void fixedCapacitySortedArrayInitializeIterator(struct FixedCapacitySortedArray* fixedCapacitySortedArray, struct FixedCapacitySortedArrayIterator* iterator) {
	iterator->iterator.hasNext = &hasNext;
	iterator->iterator.next = &next;
	iterator->iterator.transformValueBeforeNextReturn = NULL;

	iterator->nextIndex = 0;
	iterator->fixedCapacitySortedArray = fixedCapacitySortedArray;
}
