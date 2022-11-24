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

#include "util/string_stream_reader.h"
#include "util/math_utils.h"

static ssize_t read(struct StringStreamReader* stringStreamReader, void* buffer, size_t bufferSize, int* errorId) {
	uint8_t* castedBuffer = buffer;
	int localCount = 0;
	uint8_t c;

	while (stringStreamReader->available > 0 && (c = stringStreamReader->string[stringStreamReader->next]) != '\0' && bufferSize > localCount) {
		localCount++;
		stringStreamReader->available--;
		stringStreamReader->next++;
		*castedBuffer++ = c;
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

void stringStreamReaderInitialize(struct StringStreamReader* stringStreamReader, const char* string, size_t stringLength) {
	memset(stringStreamReader, 0, sizeof(struct StringStreamReader));
	stringStreamReader->available = stringLength;
	stringStreamReader->string = string;
	streamReaderInitialize(&stringStreamReader->streamReader, (ssize_t (*)(struct StreamReader*, void*, size_t, int*)) &read);
}
