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

#include "util/string_stream_writer.h"

static void test1(void) {
	size_t bufferSize = 128;
	char* buffer = malloc(sizeof(char) * bufferSize);
	assert(buffer != NULL);

	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, bufferSize);

	size_t result;
	const char* string;

	string = "AA";
	result = streamWriterWrite(&stringStreamWriter.streamWriter, string, strlen(string));
	assert(result == strlen(string));
	assert(streamWriterGetWrittenCharacterCount(&stringStreamWriter.streamWriter) == 2);

	string = "BB";
	result = streamWriterWrite(&stringStreamWriter.streamWriter, string, strlen(string));
	assert(result == strlen(string));
	assert(streamWriterGetWrittenCharacterCount(&stringStreamWriter.streamWriter) == 4);

	string = "CDDD";
	result = streamWriterWrite(&stringStreamWriter.streamWriter, string, 1);
	assert(result == 1);
	assert(streamWriterGetWrittenCharacterCount(&stringStreamWriter.streamWriter) == 5);

	result = stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(!result);

	string = "AABBC";
	assert(streamWriterGetWrittenCharacterCount(&stringStreamWriter.streamWriter) == strlen(string));
	assert(strcmp(string, buffer) == 0);

	free(buffer);
}

static void test2(void) {
	size_t bufferSize = 6;
	char* buffer = malloc(sizeof(char) * bufferSize);
	assert(buffer != NULL);

	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, bufferSize);

	size_t result;
	const char* string;

	string = "AAAAA";
	result = streamWriterWrite(&stringStreamWriter.streamWriter, string, strlen(string));
	assert(result == strlen(string));
	assert(streamWriterGetWrittenCharacterCount(&stringStreamWriter.streamWriter) == 5);

	string = "BB";
	result = streamWriterWrite(&stringStreamWriter.streamWriter, string, UINT_MAX);
	assert(result == 1);
	assert(streamWriterGetWrittenCharacterCount(&stringStreamWriter.streamWriter) == 6);

	result = stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(result);

	string = "AAAAA";
	assert(streamWriterGetWrittenCharacterCount(&stringStreamWriter.streamWriter) == 6);
	assert(strcmp(string, buffer) == 0);

	free(buffer);
}

static void test3(void) {
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, NULL, 0);

	size_t result;
	const char* string;

	string = "AAAAAAAAAAAAAAAAAAAAAAAAAAAA";
	result = streamWriterWrite(&stringStreamWriter.streamWriter, string, strlen(string));
	assert(result == EOF);
	assert(streamWriterGetWrittenCharacterCount(&stringStreamWriter.streamWriter) == 0);

	result = stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(!result);
}

static void test4(void) {
	#define BUFFER_SIZE 512
	char buffer[BUFFER_SIZE];
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);

	assert(stringStreamWriterGetAvailable(&stringStreamWriter) == BUFFER_SIZE);

	streamWriterWriteCharacter(&stringStreamWriter.streamWriter, 'A');
	assert(stringStreamWriterGetAvailable(&stringStreamWriter) == BUFFER_SIZE - 1);

	streamWriterWriteCharacter(&stringStreamWriter.streamWriter, 'B');
	assert(stringStreamWriterGetAvailable(&stringStreamWriter) == BUFFER_SIZE - 2);

	streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '\0');
	assert(stringStreamWriterGetAvailable(&stringStreamWriter) == BUFFER_SIZE - 3);
	assert(2 == strlen(buffer));
}

static void test5(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);

	ssize_t result = streamWriterFormat(&stringStreamWriter.streamWriter, "%d %d %d %c\n", 1, 2, 3, ' ');
	assert(result == 8);
	assert(strcmp("1 2 3  \n", buffer) == 0);
	assert(strlen(buffer) == streamWriterGetWrittenCharacterCount(&stringStreamWriter.streamWriter));
}

static void test6(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 2
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);

	streamWriterWriteCharacter(&stringStreamWriter.streamWriter, 'A');
	assert(stringStreamWriterGetAvailable(&stringStreamWriter) == BUFFER_SIZE - 1);

	streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '\0');
	assert(stringStreamWriterGetAvailable(&stringStreamWriter) == BUFFER_SIZE - 2);

	assert(EOF == streamWriterFormat(&stringStreamWriter.streamWriter, "%d %d %d %c\n", 1, 2, 3, ' '));
}

static void test7(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 16
	char buffer[BUFFER_SIZE];
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);

	ssize_t result = streamWriterWriteString(&stringStreamWriter.streamWriter, "ABCDEF", UINT_MAX);
	assert(result == 6);
	assert(stringStreamWriterGetAvailable(&stringStreamWriter) == 10);

	result = streamWriterWriteString(&stringStreamWriter.streamWriter, "123456789", 9);
	assert(result == 9);
	assert(stringStreamWriterGetAvailable(&stringStreamWriter) == 1);

	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(strcmp(buffer, "ABCDEF123456789") == 0);
	assert(strlen(buffer) == BUFFER_SIZE - 1);
	assert(stringStreamWriterGetAvailable(&stringStreamWriter) == 0);
}

int main(int argc, char** argv) {
	test1();
	test2();
	test3();
	test4();
	test5();
	test6();
	test7();

	return 0;
}
