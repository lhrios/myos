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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "user/util/buffered_stream_writer.h"
#include "user/util/file_descriptor_stream_writer.h"

static const char* filePath = "/tmp/output.txt";

static bool isFileContentEqualTo(const char* file, const char* expectedContent) {
	char* content = malloc(strlen(expectedContent) * sizeof(char) + 1);
	int fileDescriptorIndex = open(filePath, O_RDONLY);

	int result = read(fileDescriptorIndex, content, strlen(expectedContent) * sizeof(char));
	assert(result >= 0);
	content[result] = '\0';

	close(fileDescriptorIndex);

	return strcmp(expectedContent, content) == 0;
}

static void test1(void) {
	const int bufferSize = 16;
	char* buffer = malloc(bufferSize * sizeof(char));
	assert(buffer != NULL);

	int fileDescriptorIndex = open(filePath, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
	assert(fileDescriptorIndex >= 0);
	struct FileDescriptorStreamWriter fileDescriptorStreamWriter;
	fileDescriptorStreamWriterInitialize(&fileDescriptorStreamWriter, fileDescriptorIndex);

	struct BufferedStreamWriter bufferedStreamWriter;
	bufferedStreamWriterInitialize(&bufferedStreamWriter, &fileDescriptorStreamWriter.streamWriter, buffer, bufferSize, false);

	assert(streamWriterWrite(&bufferedStreamWriter.streamWriter, "ABCDEF", 6) == 6);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 6);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 0);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWrite(&bufferedStreamWriter.streamWriter, "12XXXXXXXXX", 2) == 2);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 8);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 0);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	bufferedStreamWriterFlush(&bufferedStreamWriter);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 8);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 8);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWrite(&bufferedStreamWriter.streamWriter, "345", 3) == 3);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 11);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 8);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWriteCharacter(&bufferedStreamWriter.streamWriter, '6') == 1);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 12);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 8);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	bufferedStreamWriterFlush(&bufferedStreamWriter);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 12);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 12);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	close(fileDescriptorIndex);
	free(buffer);

	assert(isFileContentEqualTo(filePath, "ABCDEF123456"));
}

static void test2(void) {
	const int bufferSize = 4;
	char* buffer = malloc(bufferSize * sizeof(char));
	assert(buffer != NULL);

	int fileDescriptorIndex = open(filePath, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
	assert(fileDescriptorIndex >= 0);
	struct FileDescriptorStreamWriter fileDescriptorStreamWriter;
	fileDescriptorStreamWriterInitialize(&fileDescriptorStreamWriter, fileDescriptorIndex);

	struct BufferedStreamWriter bufferedStreamWriter;
	bufferedStreamWriterInitialize(&bufferedStreamWriter, &fileDescriptorStreamWriter.streamWriter, buffer, bufferSize, false);

	assert(streamWriterWrite(&bufferedStreamWriter.streamWriter, "ABCDEF", 6) == 6);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 6);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 4);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWrite(&bufferedStreamWriter.streamWriter, "12XXXXXXXXX", 2) == 2);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 8);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 4);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWrite(&bufferedStreamWriter.streamWriter, "345", 3) == 3);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 11);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 8);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWriteCharacter(&bufferedStreamWriter.streamWriter, '6') == 1);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 12);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 8);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	bufferedStreamWriterFlush(&bufferedStreamWriter);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 12);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 12);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	close(fileDescriptorIndex);
	free(buffer);

	assert(isFileContentEqualTo(filePath, "ABCDEF123456"));
}

static void test3(void) {
	const int bufferSize = 4;
	char* buffer = malloc(bufferSize * sizeof(char));
	assert(buffer != NULL);

	int fileDescriptorIndex = open(filePath, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
	assert(fileDescriptorIndex >= 0);
	struct FileDescriptorStreamWriter fileDescriptorStreamWriter;
	fileDescriptorStreamWriterInitialize(&fileDescriptorStreamWriter, fileDescriptorIndex);

	struct BufferedStreamWriter bufferedStreamWriter;
	bufferedStreamWriterInitialize(&bufferedStreamWriter, &fileDescriptorStreamWriter.streamWriter, buffer, bufferSize, false);

	assert(streamWriterWriteCharacter(&bufferedStreamWriter.streamWriter, 'A') == 1);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 1);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 0);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWriteCharacter(&bufferedStreamWriter.streamWriter, 'B') == 1);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 2);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 0);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWriteCharacter(&bufferedStreamWriter.streamWriter, 'C') == 1);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 3);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 0);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWriteCharacter(&bufferedStreamWriter.streamWriter, 'D') == 1);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 4);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 0);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWriteCharacter(&bufferedStreamWriter.streamWriter, 'E') == 1);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 5);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 4);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	bufferedStreamWriterFlush(&bufferedStreamWriter);
	assert(streamWriterGetWrittenCharacterCount(&bufferedStreamWriter.streamWriter) == 5);
	assert(streamWriterMayAcceptMoreData(&bufferedStreamWriter.streamWriter));
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 5);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	close(fileDescriptorIndex);
	free(buffer);

	assert(isFileContentEqualTo(filePath, "ABCDE"));
}

int main(int argc, char** argv) {
	test1();
	test2();
	test3();

	return 0;
}
