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
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include <myos.h>

#include "util/math_utils.h"

#include "test/integration_test.h"

static const char* const FILE_CONTENT = "Mussum Ipsum, cacilds vidis litro abertis. "
		"Cevadis im ampola pa arma uma pindureta. Praesent vel viverra nisi. "
		"Mauris aliquet nunc non turpis scelerisque, eget. Em pé sem cair, deitado sem dormir, sentado sem cochilar e fazendo pose. "
		"Atirei o pau no gatis, per gatis num morreus.";

static void test1(const char* testCaseName) {
	char* fileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(fileName != NULL);

	int fileDescriptorIndex;
	int result;

	fileDescriptorIndex = open(fileName, O_CREAT | O_WRONLY);
	assert(fileDescriptorIndex >= 0);
	result = write(fileDescriptorIndex, FILE_CONTENT, strlen(FILE_CONTENT));
	assert(result == strlen(FILE_CONTENT));
	result = close(fileDescriptorIndex);
	assert(result == 0);

	fileDescriptorIndex = open(fileName, O_CREAT | O_EXCL | O_RDONLY);
	assert(fileDescriptorIndex == -1);
	assert(errno = EEXIST);

	fileDescriptorIndex = open(fileName, O_CREAT | O_RDONLY);
	size_t length = strlen(FILE_CONTENT);
	char* buffer = malloc(sizeof(char) * length);
	result = read(fileDescriptorIndex, buffer, length);
	assert(result == length);
	strncmp(FILE_CONTENT, buffer, length);
	result = close(fileDescriptorIndex);
	assert(result == 0);

	free(buffer);
	free(fileName);
}

static void test2(const char* testCaseName) {
	char* fileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(fileName != NULL);

	const int BLOCK_COUNT = 32;

	int fileDescriptorIndex;
	int result;

	fileDescriptorIndex = open(fileName, O_CREAT | O_WRONLY);
	assert(fileDescriptorIndex >= 0);
	for (int i = 0; i < BLOCK_COUNT; i++) {
		result = write(fileDescriptorIndex, FILE_CONTENT, strlen(FILE_CONTENT));
		assert(result == strlen(FILE_CONTENT));
	}
	result = close(fileDescriptorIndex);
	assert(result == 0);

	const int ALIAS_COUNT = 8;
	char** aliases = malloc(sizeof(char*) * ALIAS_COUNT);
	assert(aliases != NULL);

	for (int i = 0; i < ALIAS_COUNT; i++) {
		aliases[i] = malloc(sizeof(char) * (strlen(fileName) + 3 + 1) * ALIAS_COUNT);
		assert(aliases[i] != NULL);
		sprintf(aliases[i], "%s_%.2d", fileName, i);

		result = link(fileName, aliases[i]);
		assert(result == 0);
	}

	/* Remove some aliases: */
	result = unlink(fileName);
	assert(result == 0);
	for (int i = 0; i < ALIAS_COUNT - 1; i++) {
		result = unlink(aliases[i]);
		assert(result == 0);
	}

	/* Read the file content using the only alias left: */
	char* buffer = malloc(sizeof(char) * strlen(FILE_CONTENT));
	assert(buffer != NULL);

	fileDescriptorIndex = open(aliases[ALIAS_COUNT - 1], O_RDONLY);
	assert(fileDescriptorIndex >= 0);
	for (int i = 0; i < BLOCK_COUNT; i++) {
		result = read(fileDescriptorIndex, buffer, strlen(FILE_CONTENT));
		assert(result == strlen(FILE_CONTENT));
		assert(strcmp(buffer, FILE_CONTENT) == 0);
	}
	result = close(fileDescriptorIndex);
	assert(result == 0);

	/* File shouldn't exist more: */
	result = unlink(aliases[ALIAS_COUNT - 1]);
	assert(result == 0);

	for (int i = 0; i < ALIAS_COUNT; i++) {
		fileDescriptorIndex = open(aliases[i], O_RDONLY);
		assert(fileDescriptorIndex < 0);
		assert(errno == ENOENT);
	}
	fileDescriptorIndex = open(fileName, O_RDONLY);
	assert(fileDescriptorIndex < 0);
	assert(errno == ENOENT);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	test1(argv[0]);
	test2(argv[0]);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
