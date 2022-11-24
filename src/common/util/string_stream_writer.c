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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "util/string_stream_writer.h"

static ssize_t write(struct StringStreamWriter* stringStreamWriter, const char* buffer, size_t bufferSize, int* errorId) {
	int localCount = 0;

	int next = stringStreamWriter->capacity - stringStreamWriter->available;
	while (stringStreamWriter->available > 0 && bufferSize > localCount) {
		stringStreamWriter->buffer[next++] = buffer[localCount++];
		stringStreamWriter->available--;
	}

	ssize_t result;
	if (bufferSize > 0) {
		if (localCount > 0) {
			result = localCount;
		} else {
			result = EOF;
		}

	} else {
		result = 0;
	}
	return result;
}

void stringStreamWriterInitialize(struct StringStreamWriter* stringStreamWriter, char* buffer, size_t bufferSize) {
	memset(stringStreamWriter, 0, sizeof(struct StringStreamWriter));
	stringStreamWriter->available = bufferSize;
	stringStreamWriter->capacity = bufferSize;
	stringStreamWriter->buffer = buffer;
	streamWriterInitialize(&stringStreamWriter->streamWriter, (ssize_t (*)(struct StreamWriter*, const void*, size_t, int*)) &write);
}
