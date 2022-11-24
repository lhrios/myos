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

#ifndef DYNAMIC_ARRAY_UTILS_H
	#define DYNAMIC_ARRAY_UTILS_H

	struct DynamicArrayUtilsMemoryAllocatorContext {
		void* buffer;
		size_t bufferSize;
	};

	void dynamicArrayUtilsMemoryAllocatorRelease(struct DynamicArrayUtilsMemoryAllocatorContext* memoryAllocatorContext, void* pointer);
	void* dynamicArrayUtilsMemoryAllocatorResize(struct DynamicArrayUtilsMemoryAllocatorContext* memoryAllocatorContext, void* pointer, size_t size);

#endif
