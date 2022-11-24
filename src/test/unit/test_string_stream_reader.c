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
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "util/string_stream_reader.h"

static void test1(void) {
	const char* string = "ABC";
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, strlen(string));

	int character;

	assert(streamReaderPeekCharacter(&stringStreamReader.streamReader, &character) == 1);
	assert(character == 'A');
	assert(streamReaderPeekCharacter(&stringStreamReader.streamReader, &character) == 1);
	assert(character == 'A');
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderError(&stringStreamReader.streamReader) == 0);

	assert(streamReaderDiscardCurrentAndPeek(&stringStreamReader.streamReader, &character) == 1);
	assert(character == 'B');
	assert(streamReaderPeekCharacter(&stringStreamReader.streamReader, &character) == 1);
	assert(character == 'B');
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderError(&stringStreamReader.streamReader) == 0);

	assert(streamReaderDiscardCurrentAndPeek(&stringStreamReader.streamReader, &character) == 1);
	assert(character == 'C');
	assert(streamReaderReadCharacter(&stringStreamReader.streamReader, &character) == 1);
	assert(character == 'C');
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 3);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderError(&stringStreamReader.streamReader) == 0);

	assert(streamReaderPeekCharacter(&stringStreamReader.streamReader, &character) == EOF);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 3);
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderError(&stringStreamReader.streamReader) == 0);
}

static void test2(void) {
	const char* string = "";
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, ULONG_MAX);

	int character;

	assert(streamReaderPeekCharacter(&stringStreamReader.streamReader, &character) == EOF);
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(!streamReaderError(&stringStreamReader.streamReader));
}

static void test3(void) {
	const char* string = "";
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, strlen(string));

	int character;

	assert(streamReaderReadCharacter(&stringStreamReader.streamReader, &character) == EOF);
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(!streamReaderError(&stringStreamReader.streamReader));
}

static void test4(void) {
	const char* string = "";
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, strlen(string));

	int character;

	assert(streamReaderDiscardCurrentAndPeek(&stringStreamReader.streamReader, &character) == EOF);
	assert(streamReaderDiscardCurrentAndPeek(&stringStreamReader.streamReader, &character) == EOF);
	assert(streamReaderDiscardCurrentAndPeek(&stringStreamReader.streamReader, &character) == EOF);
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(!streamReaderError(&stringStreamReader.streamReader));
}

static void test5(void) {
	const char* string = "ABXXXXX";
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, 2);

	int character;

	assert(streamReaderPeekCharacter(&stringStreamReader.streamReader, &character) == 1);
	assert(character == 'A');
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(!streamReaderError(&stringStreamReader.streamReader));

	assert(streamReaderDiscardCurrentAndPeek(&stringStreamReader.streamReader, &character) == 1);
	assert(character == 'B');
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(!streamReaderError(&stringStreamReader.streamReader));

	assert(streamReaderReadCharacter(&stringStreamReader.streamReader, &character) == 1);
	assert(character == 'B');
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 2);
	assert(!streamReaderError(&stringStreamReader.streamReader));

	assert(streamReaderDiscardCurrentAndPeek(&stringStreamReader.streamReader, &character) == EOF);
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 2);
	assert(!streamReaderError(&stringStreamReader.streamReader));
}

static void test6(void) {
	const char* string = "X";
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, strlen(string));

	int character;

	assert(streamReaderReadCharacter(&stringStreamReader.streamReader, &character) == 1);
	assert(character == 'X');
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(!streamReaderError(&stringStreamReader.streamReader));

	assert(streamReaderReadCharacter(&stringStreamReader.streamReader, &character) == EOF);
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(!streamReaderError(&stringStreamReader.streamReader));
}

static void test7(void) {
	const char* string = "\xFF\xFE\xFD\xFC\xFB\xFA";
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, UINT_MAX);

	size_t bufferSize = 16;
	int character;
	char buffer[bufferSize];
	int result;

	assert(streamReaderReadCharacter(&stringStreamReader.streamReader, &character) == 1);
	assert(character == 0xFF);
	*buffer = (char) character;
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderRead(&stringStreamReader.streamReader, buffer + 1, bufferSize - 1);
	assert(result == 5);
	buffer[result + 1] = '\0';
	assert(strcmp(string, buffer) == 0);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == strlen(string));
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	assert(streamReaderRead(&stringStreamReader.streamReader, buffer + 1, bufferSize - 1) == EOF);
	assert(strcmp(string, buffer) == 0);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == strlen(string));
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	assert(streamReaderRead(&stringStreamReader.streamReader, buffer + 1, bufferSize - 1) == EOF);
	assert(strcmp(string, buffer) == 0);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == strlen(string));
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));
}

static void test8(void) {
	const char* string = "0123456789";
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, UINT_MAX);

	size_t bufferSize = 16;
	int character;
	char buffer[bufferSize];
	ssize_t result;

	result = streamReaderPeekCharacter(&stringStreamReader.streamReader, &character);
	assert(character == '0');
	*buffer = character;
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderRead(&stringStreamReader.streamReader, buffer + 1, 4);
	assert(result == 4);
	buffer[result + 1] = '\0';
	assert(strcmp("00123", buffer) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderRead(&stringStreamReader.streamReader, buffer, 6);
	assert(result == 6);
	buffer[result] = '\0';
	assert(strcmp("456789", buffer) == 0);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == strlen(string));
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderRead(&stringStreamReader.streamReader, buffer, bufferSize);
	assert(result == EOF);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == strlen(string));
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));
}

static void test9(void) {
	const char* string = "\xFF\xFE\xFD\xFC\xFB\xFA";
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, UINT_MAX);

	size_t bufferSize = 16;
	int character;
	char buffer[bufferSize];
	int result;

	result = streamReaderUndoReadCharacter(&stringStreamReader.streamReader, 'A');
	assert(result == EOF);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderPeekCharacter(&stringStreamReader.streamReader, &character);
	assert(character == 0xFF);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderUndoReadCharacter(&stringStreamReader.streamReader, 'A');
	assert(result == EOF);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderPeekCharacter(&stringStreamReader.streamReader, &character);
	assert(character == 0xFF);
	buffer[0] = character;
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderReadCharacter(&stringStreamReader.streamReader, &character);
	assert(character == 0xFF);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderReadCharacter(&stringStreamReader.streamReader, &character);
	assert(character == 0xFE);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 2);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderUndoReadCharacter(&stringStreamReader.streamReader, 0xFE);
	assert(result == 1);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderPeekCharacter(&stringStreamReader.streamReader, &character);
	assert(character == 0xFE);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderRead(&stringStreamReader.streamReader, buffer + 1, bufferSize - 1);
	assert(result == 5);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == strlen(string));
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));
	buffer[strlen(string)] = 0;
	assert(strcmp(string, buffer) == 0);

	result = streamReaderRead(&stringStreamReader.streamReader, buffer, bufferSize);
	assert(result == EOF);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == strlen(string));
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));
}

static void test10(void) {
	const char* string = "A";
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, UINT_MAX);

	int character;
	int result;

	result = streamReaderReadCharacter(&stringStreamReader.streamReader, &character);
	assert(result == 1);
	assert(character == 'A');
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderReadCharacter(&stringStreamReader.streamReader, &character);
	assert(result == EOF);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderUndoReadCharacter(&stringStreamReader.streamReader, 'A');
	assert(result == 1);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderPeekCharacter(&stringStreamReader.streamReader, &character);
	assert(result == 1);
	assert(character == 'A');
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 0);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderReadCharacter(&stringStreamReader.streamReader, &character);
	assert(result == 1);
	assert(character == 'A');
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));

	result = streamReaderPeekCharacter(&stringStreamReader.streamReader, &character);
	assert(result == EOF);
	assert(streamReaderGetConsumedCharacterCount(&stringStreamReader.streamReader) == 1);
	assert(!streamReaderMayHasMoreDataAvailable(&stringStreamReader.streamReader));
	assert(!streamReaderError(&stringStreamReader.streamReader));
}

int main(int argc, char** argv) {
	test1();
	test2();
	test3();
	test4();
	test5();
	test6();
	test7();
	test8();
	test9();
	test10();

	return 0;
}
