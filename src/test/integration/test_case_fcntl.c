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

#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

static const char* const FILE_CONTENT_1 = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
static const char* const FILE_CONTENT_2 = "x7cGd8a8eBHMEAKo7U6b3aC2UT1waiI94qNqEfYMzpJeplJeWpx6vcfmnQ4BKC1RC5HNKekDbMfGAl9m";

static int fileDescriptorIndex1;
static int fileDescriptorIndex2;

static void doChildAssert(int fileDescriptorIndex, const char* fileContent) {
	int result;

	result = fcntl(fileDescriptorIndex, F_GETFD);
	assert(result == FD_CLOEXEC);

	char* buffer;
	buffer = malloc(sizeof(char) * strlen(fileContent));
	assert(buffer != NULL);
	result = lseek(fileDescriptorIndex, 0, SEEK_SET);
	assert(result == 0);

	result = read(fileDescriptorIndex, buffer, strlen(fileContent));
	assert(result == strlen(fileContent));
	assert(strncmp(buffer, fileContent, strlen(fileContent)) == 0);

	free(buffer);
}

static void doChild(const char* testCaseName) {
	doChildAssert(fileDescriptorIndex1, FILE_CONTENT_1);
	doChildAssert(fileDescriptorIndex2, FILE_CONTENT_2);

	char* argv[3];
	char buffer1[64];
	char buffer2[64];
	sprintf(buffer1, "%d", fileDescriptorIndex1);
	sprintf(buffer2, "%d", fileDescriptorIndex2);
	argv[0] = buffer1;
	argv[1] = buffer2;
	argv[2] = NULL;

	execvp(INTEGRATION_TEST_EXECUTABLES_PATH "executable_assert_file_descriptors_closed", argv);

	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	int result;

	{
		char* fileName = integrationTestCreateTemporaryFileName(argv[0]);
		assert(fileName != NULL);

		fileDescriptorIndex1 = open(fileName, O_CREAT | O_RDWR);
		assert(fileDescriptorIndex1 >= 0);
		result = write(fileDescriptorIndex1, FILE_CONTENT_1, strlen(FILE_CONTENT_1));
		assert(result == strlen(FILE_CONTENT_1));

		result = fcntl(fileDescriptorIndex1, F_SETFD, FD_CLOEXEC);
		assert(result == 0);

		result = fcntl(fileDescriptorIndex1, F_GETFD);
		assert(result == FD_CLOEXEC);

		free(fileName);
	}

	{
		char* fileName = integrationTestCreateTemporaryFileName(argv[0]);
		assert(fileName != NULL);

		fileDescriptorIndex2 = open(fileName, O_CREAT | O_RDWR | O_CLOEXEC);
		assert(fileDescriptorIndex2 >= 0);
		result = write(fileDescriptorIndex2, FILE_CONTENT_2, strlen(FILE_CONTENT_2));
		assert(result == strlen(FILE_CONTENT_2));

		free(fileName);
	}

	pid_t childProcessId = fork();
	if (childProcessId == 0) {
		doChild(argv[0]);

	} else if (childProcessId > 0) {
		int status;
		pid_t waitResult = wait(&status);
		assert(waitResult == childProcessId);
		assert(WIFEXITED(status));
		assert(WEXITSTATUS(status) == EXIT_SUCCESS);

	} else {
		assert(false);
	}

	result = close(fileDescriptorIndex1);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
