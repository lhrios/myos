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

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "user/util/buffered_stream_writer.h"

#include "util/math_utils.h"

static ssize_t flushLineBuffered(struct BufferedStreamWriter* bufferedStreamWriter, int* errorId) {
	const char* castedBuffer = bufferedStreamWriter->buffer;
	for (int i = bufferedStreamWriter->next - 1; i >= 0; i--) {
		if (castedBuffer[i] == '\n') {
			ssize_t result = streamWriterWrite(bufferedStreamWriter->delegate, bufferedStreamWriter->buffer, i + 1);
			if (result == EOF) {
				*errorId = streamWriterError(bufferedStreamWriter->delegate);
			} else {
				bufferedStreamWriter->next = bufferedStreamWriter->next - i - 1;
				if (bufferedStreamWriter->next > 0) {
					memmove(bufferedStreamWriter->buffer, bufferedStreamWriter->buffer + i + 1, bufferedStreamWriter->next);
				}
			}
			return result;
		}
	}

	return 0;
}

static ssize_t write(struct BufferedStreamWriter* bufferedStreamWriter, const void* buffer, size_t bufferSize, int* errorId) {
	bool error = false;
	*errorId = 0;

	size_t totalCount = 0;

	while (bufferSize > 0 && streamWriterMayAcceptMoreData(bufferedStreamWriter->delegate)) {
		if (bufferedStreamWriter->next == bufferedStreamWriter->bufferSize) {
			ssize_t result = streamWriterWrite(bufferedStreamWriter->delegate, bufferedStreamWriter->buffer, bufferedStreamWriter->bufferSize);
			if (result == EOF) {
				error = true;
				*errorId = streamWriterError(bufferedStreamWriter->delegate);
				break;

			} else {
				bufferedStreamWriter->next = 0;
			}
		}

		size_t count = mathUtilsMin(bufferSize, bufferedStreamWriter->bufferSize - bufferedStreamWriter->next);
		memcpy(bufferedStreamWriter->buffer + bufferedStreamWriter->next, buffer, count);
		bufferedStreamWriter->next += count;
		buffer += count;
		bufferSize -= count;
		totalCount += count;
	}

	if (bufferedStreamWriter->lineBuffered) {
		ssize_t result = flushLineBuffered(bufferedStreamWriter, errorId);
		if (result == EOF) {
			error = true;
		}
	}

	if (error) {
		return EOF;

	} else {
		return totalCount;
	}
}

ssize_t bufferedStreamWriterFlush(struct BufferedStreamWriter* bufferedStreamWriter) {
	bufferedStreamWriter->streamWriter.errorId = 0;

	if (bufferedStreamWriter->next != 0) {
		ssize_t result = streamWriterWrite(bufferedStreamWriter->delegate, bufferedStreamWriter->buffer, bufferedStreamWriter->next);
		if (result == EOF) {
			bufferedStreamWriter->streamWriter.errorId = streamWriterError(bufferedStreamWriter->delegate);
		} else {
			bufferedStreamWriter->next = 0;

		}
		return result;

	} else {
		return 0;
	}
}

void bufferedStreamWriterInitialize(struct BufferedStreamWriter* bufferedStreamWriter, struct StreamWriter* delegate, void* buffer,
		size_t bufferSize, bool lineBuffered) {
	memset(bufferedStreamWriter, 0, sizeof(struct BufferedStreamWriter));
	bufferedStreamWriter->delegate = delegate;
	bufferedStreamWriter->buffer = buffer;
	bufferedStreamWriter->bufferSize = bufferSize;
	bufferedStreamWriter->lineBuffered = lineBuffered;
	streamWriterInitialize(&bufferedStreamWriter->streamWriter, (ssize_t (*)(struct StreamWriter*, const void*, size_t, int*)) &write);
}
