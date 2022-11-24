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

#include "user/util/dynamic_array.h"
#include "user/util/dynamic_array_utils.h"

void dynamicArrayUtilsMemoryAllocatorRelease(struct DynamicArrayUtilsMemoryAllocatorContext* memoryAllocatorContext, void* pointer) {
	free(pointer);
}

void* dynamicArrayUtilsMemoryAllocatorResize(struct DynamicArrayUtilsMemoryAllocatorContext* memoryAllocatorContext, void* pointer, size_t size) {
	if (memoryAllocatorContext != NULL) {
		/* Is it the first call? */
		if (pointer == NULL) {
			pointer = memoryAllocatorContext->buffer;
		} else {
			assert(pointer == memoryAllocatorContext->buffer);
		}

		if (memoryAllocatorContext->buffer != NULL && memoryAllocatorContext->bufferSize >= size) {
			return memoryAllocatorContext->buffer;
		} else {
			void* result = realloc(memoryAllocatorContext->buffer, size);
			if (result != NULL) {
				memoryAllocatorContext->buffer = result;
				memoryAllocatorContext->bufferSize = size;
			}
			return result;
		}

	} else {
		return realloc(pointer, size);
	}
}
