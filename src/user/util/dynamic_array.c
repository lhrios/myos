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

#include "user/util/dynamic_array.h"

void dynamicArrayInitialize(struct DynamicArray* dynamicArray, size_t elementSize, void* memoryAllocatorContext,
		void (*memoryAllocatorRelease)(void*, void*), void* (*memoryAllocatorResize)(void*, void*, size_t)) {
	dynamicArray->array = NULL;
	dynamicArray->capacity = 0;
	dynamicArray->size = 0;
	dynamicArray->elementSize = elementSize;
	dynamicArray->memoryAllocatorContext = memoryAllocatorContext;
	dynamicArray->memoryAllocatorRelease = memoryAllocatorRelease;
	dynamicArray->memoryAllocatorResize = memoryAllocatorResize;
}

void* dynamicArrayInsertAfterLast(struct DynamicArray* dynamicArray, const void* element) {
	const int DEFAULT_INITIAL_CAPACITY = 128;

	if (dynamicArray->capacity == dynamicArray->size) {
		int newCapacity;
		if (dynamicArray->capacity == 0) {
			newCapacity = DEFAULT_INITIAL_CAPACITY;
		} else {
			newCapacity = dynamicArray->capacity * 2;
		}

		void* newArray = dynamicArray->memoryAllocatorResize(dynamicArray->memoryAllocatorContext,
				dynamicArray->array, newCapacity * dynamicArray->elementSize);
		if (newArray == NULL) {
			return NULL;
		}
		dynamicArray->array = newArray;
		dynamicArray->capacity = newCapacity;
	}

	void* newElementPointer = dynamicArray->array + dynamicArray->size * dynamicArray->elementSize;
	memcpy(newElementPointer, element, dynamicArray->elementSize);
	dynamicArray->size++;

	return newElementPointer;
}

void dynamicArrayCopy(struct DynamicArray* dynamicArray, void* destination) {
	memcpy(destination, dynamicArray->array, dynamicArray->size * dynamicArray->elementSize);
}

void dynamicArrayClear(struct DynamicArray* dynamicArray, bool release) {
	if (dynamicArray->array != NULL) {
		if (release) {
			dynamicArray->memoryAllocatorRelease(dynamicArray->memoryAllocatorContext, dynamicArray->array);
			dynamicArray->array = NULL;
			dynamicArray->capacity = 0;
		}

		dynamicArray->size = 0;
	} else {
		assert(dynamicArray->size == 0);
	}
}

