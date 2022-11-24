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

#include "util/ring_buffer.h"
#include "util/math_utils.h"

#include <assert.h>
#include <string.h>

void ringBufferInitialize(struct RingBuffer *ringBuffer, void* buffer, int capacity) {
	ringBuffer->capacity = capacity;
	ringBuffer->size = 0;
	ringBuffer->begin = 0;
	ringBuffer->buffer = buffer;
}

int ringBufferCopy(struct RingBuffer *ringBuffer, void* buffer, int bufferSize, int offset) {
	int size;
	if (offset < 0) {
		if (-offset >= ringBuffer->size) {
			size = ringBuffer->size;
			offset = 0;
		} else {
			size = -offset;
			offset = ringBuffer->size + offset;
			offset = mathUtilsMax(0, offset);
		}

	} else {
		size = ringBuffer->size - offset;
		size = mathUtilsMax(0, size);
	}

	size = mathUtilsMin(size, bufferSize);

	int remainingOffset;
	int begin = ringBuffer->begin + offset;

	int firstCopyCount;
	if (begin < ringBuffer->capacity) {
		firstCopyCount = ringBuffer->capacity - begin;
		firstCopyCount = mathUtilsMin(firstCopyCount, size);

		memcpy(buffer, ringBuffer->buffer + begin, firstCopyCount);
		remainingOffset = 0;

	} else {
		firstCopyCount = 0;
		remainingOffset = begin - ringBuffer->capacity;
	}

	int secondCopyCount = 0;
	if (firstCopyCount < size) {
		secondCopyCount = size - firstCopyCount;
		memcpy(buffer + firstCopyCount, ringBuffer->buffer + remainingOffset, secondCopyCount);
	}

	return firstCopyCount + secondCopyCount;
}

void ringBufferWrite(struct RingBuffer *ringBuffer, const void* buffer, int count) {
	assert(ringBuffer->size <= ringBuffer->capacity);

	int offset = 0;
	if (count > ringBuffer->capacity) {
		offset = count - ringBuffer->capacity;
		count = ringBuffer->capacity;
	}

	int end;
	/* Does it need to overwrite? */
	if (count <= ringBuffer->capacity - ringBuffer->size) {
		end = ringBuffer->begin + ringBuffer->size;
		if (end >= ringBuffer->capacity) {
			end -= ringBuffer->capacity;
		}
		ringBuffer->size += count;

	} else {
		end = ringBuffer->begin;
		ringBuffer->begin += count;
		ringBuffer->begin %= ringBuffer->capacity;
	}

	int firstCopyCount = ringBuffer->capacity - end;

	memcpy(ringBuffer->buffer + end, buffer + offset, mathUtilsMin(firstCopyCount, count));
	if (count > firstCopyCount) {
		memcpy(ringBuffer->buffer, buffer + offset + firstCopyCount, count - firstCopyCount);
	}
}

int ringBufferRead(struct RingBuffer *ringBuffer, void* buffer, int bufferSize) {
	int copyCount = ringBufferCopy(ringBuffer, buffer, bufferSize, 0);
	assert (ringBuffer->size >= copyCount);
	if (ringBuffer->size == copyCount) {
		ringBuffer->size = 0;

	} else {
		ringBuffer->size -= copyCount;
		ringBuffer->begin += copyCount;
		ringBuffer->begin %= ringBuffer->capacity;
	}

	return copyCount;
}

void ringBufferDiscard(struct RingBuffer *ringBuffer, int count) {
	if (count < 0) {
		count = -count;
		ringBuffer->size -= count;

		if (ringBuffer->size < 0) {
			ringBuffer->size = 0;
		}

	} else if (count > 0) {
		ringBuffer->size -= count;

		if (ringBuffer->size >= 0) {
			ringBuffer->begin += count;
			ringBuffer->begin %= ringBuffer->capacity;
		} else {
			ringBuffer->size = 0;
		}
	}
}

void ringBufferOverWrite(struct RingBuffer *ringBuffer, const void* buffer, int count, int offset) {
	assert(offset <= 0);

	assert(ringBuffer->size <= ringBuffer->capacity);

	int firstCopyCount = 0;
	int secondCopyCount = 0;
	if (offset < 0) {
		if (-offset > ringBuffer->size) {
			offset = -ringBuffer->size;
		}

		int end = ringBuffer->begin + ringBuffer->size;
		if (end >= ringBuffer->capacity) {
			end -= ringBuffer->capacity;
		}

		int begin = end + offset;
		if (begin < 0) {
			begin += ringBuffer->capacity;
		}

		firstCopyCount = mathUtilsMin(count, mathUtilsMin(ringBuffer->capacity - begin, -offset));
		memcpy(ringBuffer->buffer + begin, buffer, firstCopyCount);
		secondCopyCount = mathUtilsMin(-offset - firstCopyCount, count - firstCopyCount);
		if (secondCopyCount > 0) {
			memcpy(ringBuffer->buffer, buffer + firstCopyCount, secondCopyCount);
		}
	}
	ringBufferWrite(ringBuffer, buffer + firstCopyCount + secondCopyCount, count - firstCopyCount - secondCopyCount);
}
