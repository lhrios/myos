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

#ifndef RING_BUFFER_H
	#define RING_BUFFER_H

	#include <assert.h>
	#include <stdbool.h>

	struct RingBuffer {
		int capacity;
		int size;
		int begin;
		void* buffer;
	};

	void ringBufferInitialize(struct RingBuffer* ringBuffer, void* buffer, int capacity);

	int ringBufferCopy(struct RingBuffer* ringBuffer, void* buffer, int bufferSize, int offset);

	void ringBufferWrite(struct RingBuffer* ringBuffer, const void* buffer, int count);

	void ringBufferOverWrite(struct RingBuffer* ringBuffer, const void* buffer, int count, int offset);

	int ringBufferRead(struct RingBuffer* ringBuffer, void* buffer, int bufferSize);

	inline __attribute__((always_inline)) int ringBufferRemaining(struct RingBuffer* ringBuffer) {
		return ringBuffer->capacity - ringBuffer->size;
	}

	inline __attribute__((always_inline)) bool ringBufferIsFull(struct RingBuffer* ringBuffer) {
		return ringBuffer->size == ringBuffer->capacity;
	}

	inline __attribute__((always_inline)) bool ringBufferIsEmpty(struct RingBuffer* ringBuffer) {
		return ringBuffer->size == 0;
	}

	inline __attribute__((always_inline)) int ringBufferSize(struct RingBuffer* ringBuffer) {
		return ringBuffer->size;
	}

	inline __attribute__((always_inline)) void ringBufferClear(struct RingBuffer* ringBuffer) {
		ringBuffer->size = 0;
	}

	inline __attribute__((always_inline)) void ringBufferDiscardLastWrittenBytes(struct RingBuffer* ringBuffer, int byteCount) {
		ringBuffer->size -= byteCount;
		assert(ringBuffer->size >= 0);
	}

	void ringBufferDiscard(struct RingBuffer* ringBuffer, int count);

#endif
