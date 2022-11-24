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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <myos.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

#include "util/math_utils.h"

static const char* const FILE_CONTENT_1 = "A\nB\nC\nD\nE\nF";

static void test1(const char* testCaseName) {
	char* temporaryFileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(temporaryFileName != NULL);

	int fileDescriptorIndex = open(temporaryFileName, O_CREAT | O_WRONLY);
	assert(fileDescriptorIndex >= 0);
	ssize_t result = write(fileDescriptorIndex, FILE_CONTENT_1, strlen(FILE_CONTENT_1));
	assert(result == strlen(FILE_CONTENT_1));
	{
		int result = close(fileDescriptorIndex);
		assert(result == 0);
	}

	fileDescriptorIndex = open(temporaryFileName, O_RDONLY);
	free(temporaryFileName);
	assert(fileDescriptorIndex >= 0);
	dup2(fileDescriptorIndex, STDIN_FILENO);

	char* buffer = NULL;
	size_t bufferSize = 0;

	result = getline(&buffer, &bufferSize, stdin);
	assert(result == 2);
	assert(bufferSize > 0);
	assert(buffer != NULL);
	assert(strcmp("A\n", buffer) == 0);

	result = getline(&buffer, &bufferSize, stdin);
	assert(result == 2);
	assert(bufferSize > 0);
	assert(buffer != NULL);
	assert(strcmp("B\n", buffer) == 0);

	result = getline(&buffer, &bufferSize, stdin);
	assert(result == 2);
	assert(bufferSize > 0);
	assert(buffer != NULL);
	assert(strcmp("C\n", buffer) == 0);

	result = getline(&buffer, &bufferSize, stdin);
	assert(result == 2);
	assert(bufferSize > 0);
	assert(buffer != NULL);
	assert(strcmp("D\n", buffer) == 0);

	result = getline(&buffer, &bufferSize, stdin);
	assert(result == 2);
	assert(bufferSize > 0);
	assert(buffer != NULL);
	assert(strcmp("E\n", buffer) == 0);

	result = getline(&buffer, &bufferSize, stdin);
	assert(result == 1);
	assert(bufferSize > 0);
	assert(buffer != NULL);
	assert(strcmp("F", buffer) == 0);

	free(buffer);
}

static void test2(const char* testCaseName) {
	char* temporaryFileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(temporaryFileName != NULL);

	int fileDescriptorIndex = open(temporaryFileName, O_CREAT | O_WRONLY);
	assert(fileDescriptorIndex >= 0);
	{
		int result = close(fileDescriptorIndex);
		assert(result == 0);
	}

	fileDescriptorIndex = open(temporaryFileName, O_RDONLY);
	free(temporaryFileName);
	assert(fileDescriptorIndex >= 0);
	dup2(fileDescriptorIndex, STDIN_FILENO);

	char* buffer = NULL;
	size_t bufferSize = 0;

	ssize_t result = getline(&buffer, &bufferSize, stdin);
	assert(result == EOF);
	assert(bufferSize == 0);
	assert(buffer == NULL);
}

static const char* const FILE_CONTENT_3 = "Vivamus at tincidunt elit, eget dapibus arcu. "
		"Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. "
		"Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. "
		"In mi ligula, ultrices vitae mollis at, egestas ac leo. Aliquam elementum congue vehicula. "
		"Pellentesque bibendum ligula ac libero sagittis, at imperdiet nisi vestibulum. "
		"Praesent sed urna ornare, congue orci a, congue odio. "
		"Phasellus hendrerit, ipsum a commodo consequat, ex dolor hendrerit odio, a gravida neque velit ac sapien. Mauris nec lacinia massa. "
		"Fusce risus sem, pharetra a sem vitae, posuere consectetur orci. Pellentesque ultricies leo porttitor, porta nisi ac, varius risus. "
		"Integer imperdiet erat ligula, ac pretium dolor varius ut.";

static void test3(const char* testCaseName) {
	char* temporaryFileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(temporaryFileName != NULL);

	int fileDescriptorIndex = open(temporaryFileName, O_CREAT | O_WRONLY);
	assert(fileDescriptorIndex >= 0);
	ssize_t result = write(fileDescriptorIndex, FILE_CONTENT_3, strlen(FILE_CONTENT_3));
	assert(result == strlen(FILE_CONTENT_3));
	{
		int result = close(fileDescriptorIndex);
		assert(result == 0);
	}

	fileDescriptorIndex = open(temporaryFileName, O_RDONLY);
	free(temporaryFileName);
	assert(fileDescriptorIndex >= 0);
	dup2(fileDescriptorIndex, STDIN_FILENO);

	char* buffer = malloc(sizeof(char) * 1);
	size_t bufferSize = 1;

	result = getline(&buffer, &bufferSize, stdin);
	assert(result == strlen(FILE_CONTENT_3));
	assert(bufferSize > 0);
	assert(buffer != NULL);
	assert(strcmp(FILE_CONTENT_3, buffer) == 0);

	free(buffer);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	test1(argv[0]);
	test2(argv[0]);
	test3(argv[0]);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
