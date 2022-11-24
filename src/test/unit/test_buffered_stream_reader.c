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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "user/util/buffered_stream_reader.h"
#include "user/util/file_descriptor_stream_reader.h"

#include "util/string_utils.h"
#include "util/string_stream_reader.h"

static void test1(void) {
	int fileDescriptorIndex = open("../resources/test/unit/test_buffered_stream_reader_1.txt", O_RDONLY);
	assert(fileDescriptorIndex >= 0);
	struct FileDescriptorStreamReader fileDescriptorStreamReader;
	fileDescriptorStreamReaderInitialize(&fileDescriptorStreamReader, fileDescriptorIndex);

	struct BufferedStreamReader bufferedStreamReader;
	size_t readerBufferSize = 3;
	char readerBuffer[readerBufferSize];
	bufferedStreamReaderInitialize(&bufferedStreamReader, &fileDescriptorStreamReader.streamReader, readerBuffer, readerBufferSize);

	size_t bufferSize = 32;
	char buffer[bufferSize];
	int character;
	ssize_t result;

	assert(streamReaderPeekCharacter(&bufferedStreamReader.streamReader, &character) == 1);
	assert(character == '0');
	assert(streamReaderGetConsumedCharacterCount(&bufferedStreamReader.streamReader) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&bufferedStreamReader.streamReader));
	assert(streamReaderError(&bufferedStreamReader.streamReader) == 0);

	result = streamReaderRead(&bufferedStreamReader.streamReader, buffer, bufferSize);
	assert(result == 10);
	buffer[10] = '\0';
	assert(strcmp(buffer, "0123456789") == 0);
	assert(streamReaderGetConsumedCharacterCount(&bufferedStreamReader.streamReader) == 10);
	assert(streamReaderMayHasMoreDataAvailable(&bufferedStreamReader.streamReader));
	assert(streamReaderError(&bufferedStreamReader.streamReader) == 0);

	assert(streamReaderPeekCharacter(&bufferedStreamReader.streamReader, &character) == EOF);
	assert(streamReaderGetConsumedCharacterCount(&bufferedStreamReader.streamReader) == 10);
	assert(!streamReaderMayHasMoreDataAvailable(&bufferedStreamReader.streamReader));
	assert(streamReaderError(&bufferedStreamReader.streamReader) == 0);

	close(fileDescriptorIndex);
}

static void test2() {
	const char* string = "The quick brown fox jumps over the lazy dog";
	char* modifiableString = strdup(string);
	assert(modifiableString != NULL);

	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, modifiableString, strlen(modifiableString));

	const size_t bufferSize = 8;
	char* buffer = malloc(sizeof(char) * bufferSize);
	assert(buffer);

	struct BufferedStreamReader bufferedStreamReader;
	bufferedStreamReaderInitialize(&bufferedStreamReader, &stringStreamReader.streamReader, buffer, bufferSize);

	int character;
	ssize_t result;

	result = streamReaderReadCharacter(&bufferedStreamReader.streamReader, &character);
	assert(result == 1);
	assert(character == 'T');
	assert(bufferedStreamReaderAvailable(&bufferedStreamReader) == bufferSize - 1);

	result = streamReaderReadCharacter(&bufferedStreamReader.streamReader, &character);
	assert(result == 1);
	assert(character == 'h');
	assert(!streamReaderIsNextCharacterBuffered(&bufferedStreamReader.streamReader));
	assert(bufferedStreamReaderAvailable(&bufferedStreamReader) == bufferSize - 2);

	bufferedStreamReaderDiscardBuffered(&bufferedStreamReader);
	assert(bufferedStreamReaderAvailable(&bufferedStreamReader) == 0);

	stringUtilsToUpperCase(modifiableString);

	result = streamReaderReadCharacter(&bufferedStreamReader.streamReader, &character);
	assert(result == 1);
	assert(character == 'K');

	result = streamReaderReadCharacter(&bufferedStreamReader.streamReader, &character);
	assert(result == 1);
	assert(character == ' ');

	result = streamReaderReadCharacter(&bufferedStreamReader.streamReader, &character);
	assert(result == 1);
	assert(character == 'B');

	result = streamReaderPeekCharacter(&bufferedStreamReader.streamReader, &character);
	assert(result == 1);
	assert(character == 'R');

	{
		const size_t bufferSize = strlen(modifiableString) - (strchr(modifiableString, 'R') - modifiableString);
		char* buffer = malloc(sizeof(char) * bufferSize);
		assert(buffer != NULL);

		result = streamReaderRead(&bufferedStreamReader.streamReader, buffer, bufferSize);
		assert(result == bufferSize);
		assert(strncmp(buffer, strchr(modifiableString, 'R'), bufferSize) == 0);

		free(buffer);
	}

	assert(bufferedStreamReaderAvailable(&bufferedStreamReader) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&bufferedStreamReader.streamReader));
	result = streamReaderPeekCharacter(&bufferedStreamReader.streamReader, &character);
	assert(result == EOF);
	assert(!streamReaderMayHasMoreDataAvailable(&bufferedStreamReader.streamReader));

	free(buffer);
	free(modifiableString);
}

int main(int argc, char** argv) {
	test1();
	test2();

	return 0;
}
