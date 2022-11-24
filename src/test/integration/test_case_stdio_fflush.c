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
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <myos.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

#include "util/math_utils.h"

static void test1(const char* testCaseName) {
	char* fileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(fileName != NULL);

	const char* content = "Ta deprimidis, eu conheco uma cachacis que pode alegrar sua vidis.";
	const int contentLength = strlen(content);

	{
		size_t bufferSize = 128;
		char* buffer = malloc(sizeof(char) * bufferSize);
		assert(buffer != NULL);

		FILE* file = fopen(fileName, "w");
		assert(file != NULL);

		int result = setvbuf(file, buffer, _IOFBF, bufferSize);
		assert(result == 0);

		result = fwrite(content, sizeof(char), contentLength, file);
		assert(result = contentLength);

		struct stat statInstance;
		result = fstat(fileno(file), &statInstance);
		assert(result == 0);
		assert(statInstance.st_size == 0);

		assert(fflush(file) == 0);

		result = fstat(fileno(file), &statInstance);
		assert(result == 0);
		assert(statInstance.st_size == contentLength);

		fclose(file);
	}

	{
		size_t bufferSize = 128;
		char* buffer = malloc(sizeof(char) * bufferSize);
		assert(buffer != NULL);

		FILE* fileToRead = fopen(fileName, "r");
		assert(fileToRead != NULL);

		int result = setvbuf(fileToRead, buffer, _IOFBF, bufferSize);
		assert(result == 0);

		FILE* fileToWrite = fopen(fileName, "r+");
		assert(fileToWrite != NULL);

		assert(fgetc(fileToRead) == 'T');
		assert(fgetc(fileToRead) == 'a');
		assert(fgetc(fileToRead) == ' ');

		for (int i = 0; i < contentLength; i++) {
			assert(fputc(toupper(content[i]), fileToWrite) != EOF);
		}
		fclose(fileToWrite);

		assert(fgetc(fileToRead) == 'd');
		assert(fgetc(fileToRead) == 'e');

		assert(fflush(fileToRead) == 0);
		assert(ftell(fileToRead) == 5);

		assert(fgetc(fileToRead) == 'P');
		assert(fgetc(fileToRead) == 'R');

		fclose(fileToRead);
	}
}

static void doWritingTest(bool noBuffer) {
	struct sigaction action;
	action.sa_handler = SIG_IGN;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	int result;
	result = sigaction(SIGPIPE, &action, NULL);
	assert(result == 0);

	int pipeFileDescriptorIndexes[2];
	result = pipe(pipeFileDescriptorIndexes);
	assert(result == 0);

	close(pipeFileDescriptorIndexes[0]);

	FILE* file = fdopen(pipeFileDescriptorIndexes[1], "w");
	if (file == NULL) {
		perror(NULL);
	}
	assert(file != NULL);

	if (noBuffer) {
		result = setvbuf(file, NULL, _IONBF, 0);
		assert(result == 0);
	}

	result = fprintf(file, "Testing...");
	if (noBuffer) {
		assert(result < 0);
		assert(ferror(file));
	} else {
		assert(result == 10);
		assert(!ferror(file));
		fflush(file);
		assert(ferror(file));
	}

	fclose(file);

	close(pipeFileDescriptorIndexes[1]);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	test1(argv[0]);
	doWritingTest(true);
	doWritingTest(false);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
