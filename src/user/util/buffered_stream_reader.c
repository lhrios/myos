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
#include <stdio.h>

#include "user/util/buffered_stream_reader.h"

#include "util/math_utils.h"

static ssize_t read(struct BufferedStreamReader* bufferedStreamReader, void* buffer, size_t bufferSize, int* errorId) {
	*errorId = 0;

	size_t totalCount = 0;

	while (bufferSize > 0) {
		if (bufferedStreamReader->available == 0) {
			ssize_t result = streamReaderRead(bufferedStreamReader->delegate, bufferedStreamReader->buffer, bufferedStreamReader->bufferSize);
			if (result == EOF) {
				break;

			} else {
				bufferedStreamReader->available = result;
				bufferedStreamReader->next = 0;
			}
		}

		size_t count = mathUtilsMin(bufferSize, bufferedStreamReader->available);
		memcpy(buffer, bufferedStreamReader->buffer + bufferedStreamReader->next, count);
		bufferedStreamReader->available -= count;
		bufferedStreamReader->next += count;
		buffer += count;
		bufferSize -= count;
		totalCount += count;
	}

	if (totalCount == 0 && bufferSize > 0) {
		if (streamReaderError(bufferedStreamReader->delegate)) {
			*errorId = streamReaderError(bufferedStreamReader->delegate);
		}
		return EOF;

	} else {
		return totalCount;
	}
}

void bufferedStreamReaderInitialize(struct BufferedStreamReader* bufferedStreamReader, struct StreamReader* delegate, void* buffer, size_t bufferSize) {
	memset(bufferedStreamReader, 0, sizeof(struct BufferedStreamReader));
	bufferedStreamReader->delegate = delegate;
	bufferedStreamReader->buffer = buffer;
	bufferedStreamReader->bufferSize = bufferSize;
	streamReaderInitialize(&bufferedStreamReader->streamReader, (ssize_t (*)(struct StreamReader*, void*, size_t, int*)) &read);
}
