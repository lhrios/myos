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

#ifndef FIXED_CAPACITY_SORTED_ARRAY_H
	#define FIXED_CAPACITY_SORTED_ARRAY_H

	#include <stdbool.h>
	#include <stddef.h>

	#include "util/iterator.h"

	struct FixedCapacitySortedArray {
		void* elements;
		int size;
		int capacity;
		size_t elementSize;

		int (*elementKeyComparator)(const void*, const void*);
		const void* (*elementKeyExtractor)(const void*);
	};

	struct FixedCapacitySortedArrayIterator {
		struct Iterator iterator;
		struct FixedCapacitySortedArray* fixedCapacitySortedArray;
		int nextIndex;
	};

	void fixedCapacitySortedArrayInitialize(struct FixedCapacitySortedArray* fixedCapacitySortedArray, size_t elementSize,
			void* array, int arrayCapacityInBytes, int (*elementKeyComparator)(const void*, const void*), const void* (*elementKeyExtractor)(const void*));
	bool fixedCapacitySortedArrayInsert(struct FixedCapacitySortedArray* fixedCapacitySortedArray, const void* element);
	void* fixedCapacitySortedArraySearch(struct FixedCapacitySortedArray* fixedCapacitySortedArray, const void* elementKey);
	bool fixedCapacitySortedArrayRemove(struct FixedCapacitySortedArray* fixedCapacitySortedArray, const void* elementKey);
	inline __attribute__((always_inline)) int fixedCapacitySortedArraySize(struct FixedCapacitySortedArray* fixedCapacitySortedArray) {
		return fixedCapacitySortedArray->size;
	}
	inline __attribute__((always_inline)) int fixedCapacitySortedArrayRemaining(struct FixedCapacitySortedArray* fixedCapacitySortedArray) {
		return fixedCapacitySortedArray->capacity - fixedCapacitySortedArray->size;
	}
	void* fixedCapacitySortedArrayGet(struct FixedCapacitySortedArray* fixedCapacitySortedArray, int index);
	void fixedCapacitySortedArrayInitializeIterator(struct FixedCapacitySortedArray* fixedCapacitySortedArray, struct FixedCapacitySortedArrayIterator* iterator);
	inline __attribute__((always_inline)) void fixedCapacitySortedArrayClear(struct FixedCapacitySortedArray* fixedCapacitySortedArray) {
		fixedCapacitySortedArray->size = 0;
	}

#endif
