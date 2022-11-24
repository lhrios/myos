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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "user/util/file_descriptor_stream_reader.h"

#include "util/math_utils.h"

static ssize_t localRead(struct FileDescriptorStreamReader* fileDescriptorStreamReader, void* buffer, size_t bufferSize, int* errorId) {
	ssize_t result = read(fileDescriptorStreamReader->fileDescriptorIndex, buffer, bufferSize);

	if (result == 0 && bufferSize > 0) {
		return EOF;

	} else if (result < 0) {
		*errorId = errno;
		return EOF;

	} else {
		return result;
	}
}

void fileDescriptorStreamReaderInitialize(struct FileDescriptorStreamReader* fileDescriptorStreamReader, int fileDescriptorIndex) {
	memset(fileDescriptorStreamReader, 0, sizeof(struct FileDescriptorStreamReader));
	fileDescriptorStreamReader->fileDescriptorIndex = fileDescriptorIndex;
	streamReaderInitialize(&fileDescriptorStreamReader->streamReader, (ssize_t (*)(struct StreamReader*, void*, size_t, int*)) &localRead);
}
