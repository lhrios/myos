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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "util/formatter.h"
#include "util/stream_writer.h"

void streamWriterInitialize(struct StreamWriter* streamWriter, ssize_t (*write)(struct StreamWriter*, const void*, size_t, int*)) {
	memset(streamWriter, 0, sizeof(struct StreamWriter));
	streamWriter->write = write;
}

_Static_assert(sizeof(char) == 1, "Expecting char with 1 byte.");
_Static_assert(sizeof(uint8_t) == 1, "Expecting uint8_t with 1 byte.");

ssize_t streamWriterWrite(struct StreamWriter* streamWriter, const void* buffer, size_t bufferSize) {
	ssize_t result;

	if (bufferSize > 0) {
		result = streamWriter->write(streamWriter, buffer, bufferSize, &streamWriter->errorId);
		if (result >= 0) {
			streamWriter->writtenCharacterCount += result;
			streamWriter->reachedEnd = bufferSize > result;
		} else {
			streamWriter->reachedEnd = false;
			result = EOF;
		}

	} else {
		streamWriter->reachedEnd = false;
		streamWriter->errorId = 0;
		result = 0;
	}

	return result;
}

ssize_t streamWriterWriteCharacter(struct StreamWriter* streamWriter, int character) {
	uint8_t castedCharacter = (uint8_t) character;
	return streamWriterWrite(streamWriter, &castedCharacter, sizeof(uint8_t));
}

ssize_t streamWriterWriteString(struct StreamWriter* streamWriter, const char* string, size_t length) {
	ssize_t actualLength = 0;
	while (actualLength < length && string[actualLength] != '\0') {
		actualLength++;
	}

	return streamWriterWrite(streamWriter, string, actualLength);
}

ssize_t streamWriterVaFormat(struct StreamWriter* streamWriter, const char* format, va_list ap) {
	return formatterFormat(streamWriter, format, ap, NULL);
}

ssize_t streamWriterFormat(struct StreamWriter* streamWriter, const char* format, ...) {
	va_list ap;

	va_start(ap, format);
	ssize_t result = streamWriterVaFormat(streamWriter, format, ap);
	va_end(ap);
	return result;
}
