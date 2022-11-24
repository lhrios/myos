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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "util/stream_reader.h"

void streamReaderInitialize(struct StreamReader* streamReader, ssize_t (*read)(struct StreamReader*, void*, size_t, int*)) {
	memset(streamReader, 0, sizeof(struct StreamReader));
	streamReader->nextCharacter = -1;
	streamReader->read = read;
}

_Static_assert(sizeof(char) == 1, "Expecting char with 1 byte.");
_Static_assert(sizeof(uint8_t) == 1, "Expecting uint8_t with 1 byte.");

ssize_t streamReaderReadCharacter(struct StreamReader* streamReader, int* character) {
	if (streamReader->nextCharacter != -1) {
		*character = streamReader->nextCharacter;
		streamReader->nextCharacter = -1;
		streamReader->consumedCharacterCount++;
		streamReader->reachedEnd = false;
		streamReader->errorId = 0;
		return 1;

	} else {
		uint8_t value;
		ssize_t result = streamReader->read(streamReader, &value, 1, &streamReader->errorId);
		if (result > 0) {
			assert(result == 1);
			*character = value;
			streamReader->consumedCharacterCount++;
			streamReader->reachedEnd = false;
			streamReader->errorId = 0;
			return result;

		} else if (streamReader->errorId) {
			streamReader->reachedEnd = false;
			return EOF;

		} else {
			streamReader->reachedEnd = true;
			streamReader->errorId = 0;
			return EOF;
		}
	}
}

ssize_t streamReaderUndoReadCharacter(struct StreamReader* streamReader, int character) {
	if (streamReader->nextCharacter == -1 && streamReader->consumedCharacterCount >= 1 && character != EOF) {
		streamReader->nextCharacter = character;
		streamReader->consumedCharacterCount--;
		streamReader->reachedEnd = false;
		streamReader->errorId = 0;
		return 1;

	} else {
		return EOF;
	}
}

ssize_t streamReaderPeekCharacter(struct StreamReader* streamReader, int* character) {
	if (streamReader->nextCharacter != -1) {
		*character = streamReader->nextCharacter;
		return 1;

	} else {
		uint8_t value;
		ssize_t result = streamReader->read(streamReader, &value, 1, &streamReader->errorId);
		if (result > 0) {
			assert(result == 1);
			streamReader->nextCharacter = value;
			streamReader->reachedEnd = false;
			streamReader->errorId = 0;
			*character = value;
			return result;

		} else if (streamReader->errorId) {
			streamReader->reachedEnd = false;
			return EOF;

		} else {
			streamReader->reachedEnd = true;
			streamReader->errorId = 0;
			return EOF;
		}
	}
}

ssize_t streamReaderRead(struct StreamReader* streamReader, void* buffer, size_t bufferSize) {
	ssize_t result;

	if (bufferSize > 0) {
		ssize_t localCount1 = 0;
		uint8_t* castedBuffer = buffer;
		if (streamReader->nextCharacter != -1) {
			*castedBuffer++ = (uint8_t) streamReader->nextCharacter;
			localCount1++;
			bufferSize--;
			streamReader->nextCharacter = -1;
			streamReader->consumedCharacterCount++;
		}

		ssize_t localCount2 = streamReader->read(streamReader, castedBuffer, bufferSize, &streamReader->errorId);
		if (localCount1 + localCount2 > 0) {
			streamReader->consumedCharacterCount += localCount2;
			streamReader->reachedEnd = false;
			streamReader->errorId = 0;
			result = localCount1 + localCount2;

		} else if (streamReader->errorId) {
			streamReader->reachedEnd = false;
			result = EOF;

		} else {
			streamReader->reachedEnd = true;
			streamReader->errorId = 0;
			result = EOF;
		}

	} else {
		streamReader->reachedEnd = false;
		streamReader->errorId = 0;
		result = 0;
	}

	return result;
}
