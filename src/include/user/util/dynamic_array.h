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

#ifndef DYNAMIC_ARRAY_H
	#define DYNAMIC_ARRAY_H

	#include <assert.h>
	#include <stdbool.h>
	#include <stddef.h>

	struct DynamicArray {
		void* array;
		size_t size;
		size_t capacity;
		size_t elementSize;

		void* memoryAllocatorContext;
		void (*memoryAllocatorRelease)(void*, void*);
		void* (*memoryAllocatorResize)(void*, void*, size_t);
	};

	void dynamicArrayInitialize(struct DynamicArray* dynamicArray, size_t elementSize, void* memoryAllocatorContext,
			void (*memoryAllocatorRelease)(void*, void*), void* (*memoryAllocatorResize)(void*, void*, size_t));

	void* dynamicArrayInsertAfterLast(struct DynamicArray* dynamicArray, const void* element);
	inline __attribute__((always_inline)) void* dynamicArrayRemoveLast(struct DynamicArray* dynamicArray) {
		assert(dynamicArray->size > 0);
		return dynamicArray->array + dynamicArray->elementSize * (--dynamicArray->size);
	}
	inline __attribute__((always_inline)) int dynamicArraySize(struct DynamicArray* dynamicArray) {
		return dynamicArray->size;
	}
	void dynamicArrayCopy(struct DynamicArray* dynamicArray, void* destination);
	void dynamicArrayClear(struct DynamicArray* dynamicArray, bool release);
	inline __attribute__((always_inline)) void* dynamicArrayGetArray(struct DynamicArray* dynamicArray) {
		return dynamicArray->array;
	}

#endif
