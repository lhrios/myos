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

#include "util/search_utils.h"

static int binarySearch(const void* keyOrValue, const void* elements, size_t elementCount, size_t elementSize, int (*compare)(const void*, const void*), const void* (*keyExtractor)(const void*)) {
	if (elementCount > 0) {
		int beginIndex = 0;
		int endIndex = elementCount - 1;

		while (beginIndex <= endIndex) {
			int middleIndex = (beginIndex + endIndex) / 2;
			const void* valueAtMiddle = elements + (elementSize * middleIndex);

			int comparisonResult;
			if (keyExtractor != NULL) {
				comparisonResult = compare(keyExtractor(valueAtMiddle), keyOrValue);
			} else {
				comparisonResult = compare(valueAtMiddle, keyOrValue);
			}


			if (comparisonResult < 0) {
				beginIndex = middleIndex + 1;
			} else if (comparisonResult > 0) {
				endIndex = middleIndex - 1;
			} else {
				return middleIndex;
			}
		}

		return -(beginIndex + 1);

	} else {
		return -1;
	}
}

int searchUtilsBinarySearchComparingElements(const void* value, const void* elements, size_t elementCount, size_t elementSize, int (*compare)(const void*, const void*)) {
	return binarySearch(value, elements, elementCount, elementSize, compare, NULL);
}

int searchUtilsBinarySearchComparingKeys(const void* key, const void* elements, size_t elementCount, size_t elementSize, int (*compare)(const void*, const void*),
		const void* (*keyExtractor)(const void*)) {
	return binarySearch(key, elements, elementCount, elementSize, compare, keyExtractor);
}
