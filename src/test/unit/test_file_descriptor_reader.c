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

#include "user/util/file_descriptor_stream_reader.h"

static void test1(void) {
	int fileDescriptorIndex = open("../resources/test/unit/test_file_descriptor_reader_1.txt", O_RDONLY);
	assert(fileDescriptorIndex >= 0);
	struct FileDescriptorStreamReader fileDescriptorStreamReader;
	fileDescriptorStreamReaderInitialize(&fileDescriptorStreamReader, fileDescriptorIndex);

	int character;

	assert(streamReaderPeekCharacter(&fileDescriptorStreamReader.streamReader, &character) == 1);
	assert(character == 'A');
	assert(streamReaderPeekCharacter(&fileDescriptorStreamReader.streamReader, &character) == 1);
	assert(character == 'A');
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	size_t bufferSize = 32;
	char buffer[bufferSize];
	size_t result = streamReaderRead(&fileDescriptorStreamReader.streamReader, buffer, bufferSize);
	assert(result == 6);
	buffer[result] = '\0';
	assert(strcmp(buffer, "ABCDEF") == 0);
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 6);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	assert(streamReaderPeekCharacter(&fileDescriptorStreamReader.streamReader, &character) == EOF);
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 6);
	assert(!streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	close(fileDescriptorIndex);
}

static void test2(void) {
	int fileDescriptorIndex = open("../resources/test/unit/test_file_descriptor_reader_2.txt", O_RDONLY);
	assert(fileDescriptorIndex >= 0);
	struct FileDescriptorStreamReader fileDescriptorStreamReader;
	fileDescriptorStreamReaderInitialize(&fileDescriptorStreamReader, fileDescriptorIndex);

	int character;

	assert(streamReaderReadCharacter(&fileDescriptorStreamReader.streamReader, &character) == 1);
	assert(character == '1');
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 1);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	assert(streamReaderPeekCharacter(&fileDescriptorStreamReader.streamReader, &character) == 1);
	assert(character == '2');
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 1);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	assert(streamReaderReadCharacter(&fileDescriptorStreamReader.streamReader, &character) == 1);
	assert(character == '2');
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 2);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	assert(streamReaderReadCharacter(&fileDescriptorStreamReader.streamReader, &character) == 1);
	assert(character == '3');
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 3);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	assert(streamReaderReadCharacter(&fileDescriptorStreamReader.streamReader, &character) == EOF);
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 3);
	assert(!streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	close(fileDescriptorIndex);
}

static void test3(void) {
	int fileDescriptorIndex = open("../resources/test/unit/test_file_descriptor_reader_3.txt", O_RDONLY);
	assert(fileDescriptorIndex >= 0);
	struct FileDescriptorStreamReader fileDescriptorStreamReader;
	fileDescriptorStreamReaderInitialize(&fileDescriptorStreamReader, fileDescriptorIndex);

	size_t bufferSize = 32;
	char buffer[bufferSize];
	int result;
	int character;

	result = streamReaderRead(&fileDescriptorStreamReader.streamReader, buffer, 3);
	assert(result == 3);
	buffer[3] = '\0';
	assert(strcmp(buffer, "XYZ") == 0);
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 3);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	result = streamReaderRead(&fileDescriptorStreamReader.streamReader, buffer, 3);
	assert(result == 3);
	buffer[3] = '\0';
	assert(strcmp(buffer, "   ") == 0);
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 6);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	result = streamReaderRead(&fileDescriptorStreamReader.streamReader, buffer, 4);
	assert(result == 4);
	buffer[4] = '\0';
	assert(strcmp(buffer, "ABC\n") == 0);
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 10);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	assert(streamReaderReadCharacter(&fileDescriptorStreamReader.streamReader, &character) == EOF);
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 10);
	assert(!streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	close(fileDescriptorIndex);
}

static void test4(void) {
	int fileDescriptorIndex = open("../resources/test/unit/test_file_descriptor_reader_4.txt", O_RDONLY);
	assert(fileDescriptorIndex >= 0);
	struct FileDescriptorStreamReader fileDescriptorStreamReader;
	fileDescriptorStreamReaderInitialize(&fileDescriptorStreamReader, fileDescriptorIndex);

	int character;

	assert(streamReaderReadCharacter(&fileDescriptorStreamReader.streamReader, &character) == 1);
	assert(character == 0xFF);
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 1);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	assert(streamReaderReadCharacter(&fileDescriptorStreamReader.streamReader, &character) == 1);
	assert(character == 0xC0);
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 2);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	assert(streamReaderReadCharacter(&fileDescriptorStreamReader.streamReader, &character) == 1);
	assert(character == 0xE0);
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 3);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	assert(streamReaderReadCharacter(&fileDescriptorStreamReader.streamReader, &character) == 1);
	assert(character == '\n');
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 4);
	assert(streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	assert(streamReaderReadCharacter(&fileDescriptorStreamReader.streamReader, &character) == EOF);
	assert(streamReaderGetConsumedCharacterCount(&fileDescriptorStreamReader.streamReader) == 4);
	assert(!streamReaderMayHasMoreDataAvailable(&fileDescriptorStreamReader.streamReader));
	assert(streamReaderError(&fileDescriptorStreamReader.streamReader) == 0);

	close(fileDescriptorIndex);
}

int main(int argc, char** argv) {
	test1();
	test2();
	test3();
	test4();

	return 0;
}
