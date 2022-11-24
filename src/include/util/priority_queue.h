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

#ifndef PRIORITY_QUEUE_H
	#define PRIORITY_QUEUE_H

	#include <assert.h>
	#include <stdbool.h>
	#include <stddef.h>

	struct PriorityQueue {
		size_t capacity;
		size_t size;
		size_t elementSize;
		void* queue;
		bool (*comparator)(const void*, const void*, void*, void*, void*);
		void* comparatorArgument1;
		void* comparatorArgument2;
		void* comparatorArgument3;
	};

	void priorityQueueInitialize(struct PriorityQueue* priorityQueue, void* queue, size_t capacity, size_t size, size_t elementSize,
			bool (*comparator)(const void*, const void*, void*, void*, void*),
			void* comparatorArgument1, void* comparatorArgument2, void* comparatorArgument3);

	bool priorityQueueInsert(struct PriorityQueue* priorityQueue, void* item);

	void priorityQueueRemove(struct PriorityQueue* priorityQueue, void* item);

	void priorityQueuePeek(struct PriorityQueue* priorityQueue, void* item);

	inline __attribute__((always_inline)) int priorityQueueSize(struct PriorityQueue* priorityQueue) {
		return priorityQueue->size;
	}

	inline __attribute__((always_inline)) void priorityQueueClear(struct PriorityQueue* priorityQueue) {
		priorityQueue->size = 0;
	}

	void priorityQueueInplaceArraySort(void* array, size_t arrayLength, size_t elementSize,
			void* comparator, bool comparatorHasArgument, void* comparatorArgument);

#endif
