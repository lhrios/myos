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

#ifndef BUFFERED_STREAM_READER_H
	#define BUFFERED_STREAM_READER_H

	#include <stdlib.h>

	#include "util/stream_reader.h"

	struct BufferedStreamReader {
		struct StreamReader streamReader;
		struct StreamReader* delegate;
		void* buffer;
		size_t bufferSize;
		size_t available;
		int next;
	};

	void bufferedStreamReaderInitialize(struct BufferedStreamReader* bufferedStreamReader, struct StreamReader* delegate, void* buffer, size_t bufferSize);

	inline __attribute__((always_inline)) int bufferedStreamReaderAvailable(struct BufferedStreamReader* bufferedStreamReader) {
		return bufferedStreamReader->available;
	}

	inline __attribute__((always_inline)) void bufferedStreamReaderDiscardBuffered(struct BufferedStreamReader* bufferedStreamReader) {
		bufferedStreamReader->available = 0;
	}

#endif
