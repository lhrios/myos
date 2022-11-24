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

#ifndef STREAM_READER_H
	#define STREAM_READER_H

	#include <stdbool.h>
	#include <stddef.h>

	#include <sys/types.h>

	struct StreamReader {
		int nextCharacter;
		ssize_t (*read)(struct StreamReader*, void*, size_t, int*);
		bool reachedEnd;
		int errorId;
		int consumedCharacterCount;
	};

	static inline __attribute__((always_inline)) bool streamReaderIsNextCharacterBuffered(struct StreamReader* streamReader) {
		return streamReader->nextCharacter != -1;
	}

	static inline __attribute__((always_inline)) bool streamReaderMayHasMoreDataAvailable(struct StreamReader* streamReader) {
		return streamReader->nextCharacter != -1 || !streamReader->reachedEnd;
	}

	static inline __attribute__((always_inline)) int streamReaderError(struct StreamReader* streamReader) {
		return streamReader->errorId;
	}

	static inline __attribute__((always_inline)) int streamReaderGetConsumedCharacterCount(struct StreamReader* streamReader) {
		return streamReader->consumedCharacterCount;
	}

	void streamReaderInitialize(struct StreamReader* streamReader,  ssize_t (*)(struct StreamReader*, void*, size_t, int*));
	ssize_t streamReaderReadCharacter(struct StreamReader* streamReader, int* character);
	ssize_t streamReaderPeekCharacter(struct StreamReader* streamReader, int* character);
	ssize_t streamReaderUndoReadCharacter(struct StreamReader* streamReader, int character);

	static inline __attribute__((always_inline)) ssize_t streamReaderDiscardCurrentAndPeek(struct StreamReader* streamReader, int* character) {
		streamReaderReadCharacter(streamReader, character);
		return streamReaderPeekCharacter(streamReader, character);
	}

	ssize_t streamReaderRead(struct StreamReader* streamReader, void* buffer, size_t bufferSize);

#endif
