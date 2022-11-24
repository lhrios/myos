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

#include <sys/stat.h>

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
	int fileDescriptorIndex = open(filePath, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
	assert(fileDescriptorIndex >= 0);
	struct FileDescriptorStreamWriter fileDescriptorStreamWriter;
	fileDescriptorStreamWriterInitialize(&fileDescriptorStreamWriter, fileDescriptorIndex);

	assert(streamWriterWrite(&fileDescriptorStreamWriter.streamWriter, "ABCDEF", 6) == 6);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 6);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWrite(&fileDescriptorStreamWriter.streamWriter, "12XXXXXXXXX", 2) == 2);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 8);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWrite(&fileDescriptorStreamWriter.streamWriter, "345", 3) == 3);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 11);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWriteCharacter(&fileDescriptorStreamWriter.streamWriter, '6') == 1);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 12);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	close(fileDescriptorIndex);

	assert(isFileContentEqualTo(filePath, "ABCDEF123456"));
}

static void test2(void) {
	int fileDescriptorIndex = open(filePath, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
	assert(fileDescriptorIndex >= 0);
	struct FileDescriptorStreamWriter fileDescriptorStreamWriter;
	fileDescriptorStreamWriterInitialize(&fileDescriptorStreamWriter, fileDescriptorIndex);

	assert(streamWriterWriteCharacter(&fileDescriptorStreamWriter.streamWriter, 'A') == 1);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 1);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWriteCharacter(&fileDescriptorStreamWriter.streamWriter, 'B') == 1);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 2);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWriteCharacter(&fileDescriptorStreamWriter.streamWriter, 'C') == 1);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 3);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	close(fileDescriptorIndex);

	assert(isFileContentEqualTo(filePath, "ABC"));
}

static void test3(void) {
	int fileDescriptorIndex = open(filePath, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
	assert(fileDescriptorIndex >= 0);
	struct FileDescriptorStreamWriter fileDescriptorStreamWriter;
	fileDescriptorStreamWriterInitialize(&fileDescriptorStreamWriter, fileDescriptorIndex);

	assert(streamWriterWriteCharacter(&fileDescriptorStreamWriter.streamWriter, 'A') == 1);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 1);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	assert(streamWriterWriteCharacter(&fileDescriptorStreamWriter.streamWriter, 'B') == 1);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 2);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	close(fileDescriptorIndex);

	int fileDescriptorIndex2 = open(filePath, O_RDONLY);
	dup2(fileDescriptorIndex2, fileDescriptorIndex);

	assert(streamWriterWriteCharacter(&fileDescriptorStreamWriter.streamWriter, 'C') == EOF);
	assert(streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter) == 2);
	assert(streamWriterError(&fileDescriptorStreamWriter.streamWriter) != 0);
	assert(streamWriterMayAcceptMoreData(&fileDescriptorStreamWriter.streamWriter));

	close(fileDescriptorIndex);

	assert(isFileContentEqualTo(filePath, "AB"));
}

int main(int argc, char** argv) {
	test1();
	test2();
	test3();

	return 0;
}
