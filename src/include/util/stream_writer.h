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

#ifndef STREAM_WRITER_H
	#define STREAM_WRITER_H

	#include <stdarg.h>
	#include <stdbool.h>
	#include <stddef.h>

	#include <sys/types.h>

	struct StreamWriter {
		ssize_t (*write)(struct StreamWriter*, const void*, size_t, int*);
		bool reachedEnd;
		int errorId;
		int writtenCharacterCount;
	};

	static inline __attribute__((always_inline)) bool streamWriterMayAcceptMoreData(struct StreamWriter* streamWriter) {
		return !streamWriter->reachedEnd;
	}

	static inline __attribute__((always_inline)) int streamWriterError(struct StreamWriter* streamWriter) {
		return streamWriter->errorId;
	}

	static inline __attribute__((always_inline)) int streamWriterGetWrittenCharacterCount(struct StreamWriter* streamWriter) {
		return streamWriter->writtenCharacterCount;
	}

	void streamWriterInitialize(struct StreamWriter* streamWriter,  ssize_t (*)(struct StreamWriter*, const void*, size_t, int*));
	ssize_t streamWriterWriteCharacter(struct StreamWriter* streamWriter, int character);
	ssize_t streamWriterWrite(struct StreamWriter* streamWriter, const void* buffer, size_t bufferSize);
	ssize_t streamWriterWriteString(struct StreamWriter* streamWriter, const char* string, size_t length);
	ssize_t streamWriterFormat(struct StreamWriter* streamWriter, const char* format, ...);
	ssize_t streamWriterVaFormat(struct StreamWriter* streamWriter, const char* format, va_list ap);

#endif
