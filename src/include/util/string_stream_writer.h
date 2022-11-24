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

#ifndef STRING_STREAM_WRITER_H
	#define STRING_STREAM_WRITER_H

	#include <stdlib.h>

	#include "util/stream_writer.h"

	struct StringStreamWriter {
		struct StreamWriter streamWriter;
		char* buffer;
		size_t capacity;
		size_t available;
	};

	inline __attribute__((always_inline)) size_t stringStreamWriterGetAvailable(struct StringStreamWriter* stringStreamWriter) {
		return stringStreamWriter->available;
	}

	inline __attribute__((always_inline)) const char* stringStreamWriterGetBuffer(struct StringStreamWriter* stringStreamWriter) {
		return stringStreamWriter->buffer;
	}

	static inline __attribute__((always_inline)) bool stringStreamWriterForceTerminationCharacter(struct StringStreamWriter* stringStreamWriter) {
		if (stringStreamWriter->capacity > 0) {
			if (stringStreamWriter->available > 0) {
				stringStreamWriter->buffer[stringStreamWriter->capacity - stringStreamWriter->available] = '\0';
				stringStreamWriter->available--;
			} else {
				stringStreamWriter->buffer[stringStreamWriter->capacity - 1] = '\0';
				return true;
			}
		}
		return false;
	}

	void stringStreamWriterInitialize(struct StringStreamWriter* stringStreamWriter, char* string, size_t stringLength);

	int stringStreamWriterFormat(struct StringStreamWriter* stringStreamWriter, const char* format, ...);

	inline __attribute__((always_inline)) const char* stringStreamWriterNextCharacterPointer(struct StringStreamWriter* stringStreamWriter) {
		return stringStreamWriter->buffer + (stringStreamWriter->capacity - stringStreamWriter->available);
	}

#endif
