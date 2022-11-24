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

static const char* const FILE_CONTENT = "Pellentesque ac tellus ex. Sed eu tincidunt ante. Donec orci elit, eleifend ac urna vel, scelerisque rutrum ex. "
	"Integer luctus dui a convallis scelerisque. Curabitur eget tincidunt nisl. Morbi tempus non arcu vitae venenatis. "
	"Maecenas vitae erat rhoncus, mattis erat sed, consequat dolor.";

static void test1(const char* testCaseName) {
	char* fileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(fileName != NULL);

	int fileDescriptorIndex;
	int result;

	fileDescriptorIndex = open(fileName, O_CREAT | O_TRUNC | O_WRONLY);
	assert(fileDescriptorIndex >= 0);
	result = write(fileDescriptorIndex, FILE_CONTENT, strlen(FILE_CONTENT));
	assert(result == strlen(FILE_CONTENT));
	result = close(fileDescriptorIndex);
	assert(result == 0);

	const size_t bufferSize = 256;
	char buffer[bufferSize];
	fileDescriptorIndex = open(fileName, O_RDONLY);
	assert(fileDescriptorIndex >= 0);
	result = read(fileDescriptorIndex, buffer, bufferSize);
	assert(result == bufferSize);
	assert(strncmp(FILE_CONTENT, buffer, bufferSize) == 0);
	result = truncate(fileName, 0);
	assert(result == 0);
	result = read(fileDescriptorIndex, buffer, bufferSize);
	assert(result == 0);
	assert(strncmp(FILE_CONTENT, buffer, bufferSize) == 0);
	result = close(fileDescriptorIndex);
	assert(result == 0);

	free(fileName);
}

static void test2(const char* testCaseName) {
	char* fileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(fileName != NULL);

	int fileDescriptorIndex;
	int result;

	fileDescriptorIndex = open(fileName, O_CREAT | O_TRUNC | O_WRONLY);
	assert(fileDescriptorIndex >= 0);
	result = write(fileDescriptorIndex, FILE_CONTENT, strlen(FILE_CONTENT));
	assert(result == strlen(FILE_CONTENT));
	result = close(fileDescriptorIndex);
	assert(result == 0);

	fileDescriptorIndex = open(fileName, O_RDONLY);
	assert(fileDescriptorIndex >= 0);
	result = lseek(fileDescriptorIndex, 0, SEEK_END);
	assert(result == strlen(FILE_CONTENT));
	result = close(fileDescriptorIndex);
	assert(result == 0);

	free(fileName);
}

static void test3(const char* testCaseName) {
	char* fileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(fileName != NULL);

	const size_t gapSize = 3456;

	int fileDescriptorIndex;
	int result;

	fileDescriptorIndex = open(fileName, O_CREAT | O_TRUNC | O_WRONLY);
	assert(fileDescriptorIndex >= 0);
	result = lseek(fileDescriptorIndex, gapSize, SEEK_SET);
	assert(result == gapSize);
	result = write(fileDescriptorIndex, FILE_CONTENT, strlen(FILE_CONTENT));
	assert(result == strlen(FILE_CONTENT));
	result = close(fileDescriptorIndex);
	assert(result == 0);

	struct stat statInstance;
	result = stat(fileName, &statInstance);
	assert(result == 0);
	assert(statInstance.st_size == strlen(FILE_CONTENT) + gapSize);

	char* buffer = malloc(sizeof(char) * statInstance.st_size);
	assert(buffer != NULL);
	fileDescriptorIndex = open(fileName, O_RDONLY);
	assert(fileDescriptorIndex >= 0);
	result = read(fileDescriptorIndex, buffer, statInstance.st_size);
	assert(result == statInstance.st_size);
	assert(strncmp(FILE_CONTENT, buffer + gapSize, strlen(FILE_CONTENT)) == 0);
	for (int i = 0; i < gapSize; i++) {
		assert(buffer[i] == 0);
	}
	result = close(fileDescriptorIndex);
	assert(result == 0);

	free(buffer);
	free(fileName);
}

static void test4(const char* testCaseName) {
	char* fileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(fileName != NULL);

	const size_t size = 12353;

	int fileDescriptorIndex;
	int result;

	fileDescriptorIndex = open(fileName, O_CREAT | O_WRONLY);
	assert(fileDescriptorIndex >= 0);
	result = ftruncate(fileDescriptorIndex, size);
	assert(result == 0);
	result = lseek(fileDescriptorIndex, 0, SEEK_CUR);
	assert(result == 0);
	result = close(fileDescriptorIndex);
	assert(result == 0);

	struct stat statInstance;
	result = stat(fileName, &statInstance);
	assert(result == 0);
	assert(statInstance.st_size == size);

	size_t bufferSize = size * 2;
	char* buffer = malloc(sizeof(char) * bufferSize);
	assert(buffer != NULL);
	fileDescriptorIndex = open(fileName, O_RDONLY);
	assert(fileDescriptorIndex >= 0);
	result = read(fileDescriptorIndex, buffer, bufferSize);
	assert(result == size);
	for (int i = 0; i < size; i++) {
		assert(buffer[i] == 0);
	}
	result = close(fileDescriptorIndex);
	assert(result == 0);

	free(buffer);
	free(fileName);
}

static void test5(const char* testCaseName) {
	char* fileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(fileName != NULL);

	const size_t size = strlen(FILE_CONTENT) + 95000;

	int fileDescriptorIndex;
	int result;

	fileDescriptorIndex = open(fileName, O_CREAT | O_WRONLY);
	assert(fileDescriptorIndex >= 0);
	result = write(fileDescriptorIndex, FILE_CONTENT, strlen(FILE_CONTENT));
	assert(result == strlen(FILE_CONTENT));
	result = ftruncate(fileDescriptorIndex, size);
	assert(result == 0);
	result = lseek(fileDescriptorIndex, 0, SEEK_CUR);
	assert(result == strlen(FILE_CONTENT));
	result = close(fileDescriptorIndex);
	assert(result == 0);

	struct stat statInstance;
	result = stat(fileName, &statInstance);
	assert(result == 0);
	assert(statInstance.st_size == size);

	size_t bufferSize = size * 2;
	char* buffer = malloc(sizeof(char) * bufferSize);
	assert(buffer != NULL);

	fileDescriptorIndex = open(fileName, O_RDONLY);
	assert(fileDescriptorIndex >= 0);
	result = read(fileDescriptorIndex, buffer, bufferSize);
	assert(result == size);
	assert(strncmp(buffer, FILE_CONTENT, strlen(FILE_CONTENT)) == 0);
	for (int i = strlen(FILE_CONTENT); i < size; i++) {
		assert(buffer[i] == 0);
	}
	result = close(fileDescriptorIndex);
	assert(result == 0);

	free(buffer);
	free(fileName);
}

static void doChildFillFile(const char* fileName, size_t fileSize, char content) {
	int result;
	int fileDescriptorIndex = open(fileName, O_CREAT | O_WRONLY);
	size_t bufferSize = 128;
	char* buffer = malloc(sizeof(char) * bufferSize);
	memset(buffer, content, bufferSize);

	while (true) {
		struct stat statInstance;
		result = stat(fileName, &statInstance);
		assert(result == 0);

		if (statInstance.st_size == fileSize) {
			break;
		} else {
			assert(statInstance.st_size < fileSize);
			size_t count = mathUtilsMin(bufferSize, fileSize - statInstance.st_size);
			result = write(fileDescriptorIndex, buffer, count);
			assert(result == count);
		}
	}
	result = close(fileDescriptorIndex);
	assert(result == 0);

	exit(EXIT_SUCCESS);
}

static void test6(const char* testCaseName) {
	int fileCount = 32;
	char** fileNames = malloc(sizeof(char*) * fileCount);
	assert(fileNames != NULL);
	for (int i = 0; i < fileCount; i++) {
		fileNames[i] = integrationTestCreateTemporaryFileName(testCaseName);
	}

	size_t* fileSizes = malloc(sizeof(size_t) * fileCount);
	assert(fileSizes != NULL);
	for (int i = 0; i < fileCount; i++) {
		fileSizes[i] = time(NULL) % (2 * 1024 * 1024);
	}

	for (int i = 0; i < fileCount; i++) {
		const char* fileName = fileNames[i];
		int fileSize = fileSizes[i];
		char content = ' ' + i;
		pid_t result = fork();
		assert(result >= 0);
		if (result == 0) {
			doChildFillFile(fileName, fileSize, content);
		}
	}

	int childCount = 0;
	while (childCount < fileCount) {
		int status;
		pid_t result = wait(&status);
		if (result > 0 && WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
			childCount++;
		} else {
			assert(false);
		}
	}

	for (int i = 0; i < fileCount; i++) {
		const char* fileName = fileNames[i];
		size_t fileSize = fileSizes[i];
		char content = ' ' + i;
		size_t bufferSize = 128;
		char buffer[bufferSize];
		int result;
		int fileDescriptorIndex = open(fileName, O_RDONLY);
		assert(fileDescriptorIndex >= 0);
		size_t count = 0;
		while (true) {
			result = read(fileDescriptorIndex, &buffer, bufferSize);
			assert(result >= 0);
			if (result == 0) {
				break;
			} else {
				for(int j = 0; j < result; j++) {
					assert(buffer[j] == content);
				}
			}
			count += result;
		}
		assert(count == fileSize);
		result = close(fileDescriptorIndex);
		assert(result == 0);
	}

	for (int i = 0; i < fileCount; i++) {
		const char* fileName = fileNames[i];
		int result = unlink(fileName);
		assert(result == 0);
	}

	for (int i = 0; i < fileCount; i++) {
		free(fileNames[i]);
	}
	free(fileNames);
	free(fileSizes);
}

static void test7(const char* testCaseName) {
	int fileSize = 1293984;
	int fileCount = 32;

	char* folderName = integrationTestCreateTemporaryFileName(testCaseName);
	int result = mkdir(folderName, 0);
	assert(result == 0);
	result = chdir(folderName);
	assert(result == 0);

	char** fileNames = malloc(sizeof(char*) * fileCount);
	assert(fileNames != NULL);
	char** fileAliasNames = malloc(sizeof(char*) * fileCount);
	assert(fileAliasNames != NULL);
	for (int i = 0; i < fileCount; i++) {
		fileNames[i] = integrationTestCreateTemporaryFileName(testCaseName);
		fileAliasNames[i] = integrationTestCreateTemporaryFileName(testCaseName);
		result = symlink(fileNames[i], fileAliasNames[i]);
		assert(result == 0);
	}

	for (int i = 0; i < fileCount; i++) {
		const char* fileName = fileNames[i];
		char content = ' ' + i;
		pid_t result = fork();
		assert(result >= 0);
		if (result == 0) {
			doChildFillFile(fileName, fileSize, content);
		}
	}

	int childCount = 0;
	while (childCount < fileCount) {
		int status;
		pid_t result = wait(&status);
		if (result > 0 && WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
			childCount++;
		} else {
			assert(false);
		}
	}

	for (int i = 0; i < fileCount; i++) {
		size_t bufferSize = 128;
		char buffer[bufferSize];
		result = readlink(fileAliasNames[i], buffer, bufferSize);
		assert(result > 0);
		buffer[result] = '\0';
		assert(strcmp(buffer, fileNames[i]) == 0);
	}

	for (int i = 0; i < fileCount; i++) {
		const char* fileName = fileAliasNames[i];
		char content = ' ' + i;
		size_t bufferSize = 128;
		char buffer[bufferSize];
		int fileDescriptorIndex = open(fileName, O_RDONLY);
		assert(fileDescriptorIndex >= 0);
		size_t count = 0;
		while (true) {
			result = read(fileDescriptorIndex, &buffer, bufferSize);
			assert(result >= 0);
			if (result == 0) {
				break;
			} else {
				for(int j = 0; j < result; j++) {
					assert(buffer[j] == content);
				}
			}
			count += result;
		}
		assert(count == fileSize);
		result = close(fileDescriptorIndex);
		assert(result == 0);
	}

	for (int i = 0; i < fileCount; i++) {
		result = unlink(fileNames[i]);
		assert(result == 0);
		result = unlink(fileAliasNames[i]);
		assert(result == 0);
	}

	result = chdir("..");
	assert(result == 0);

	result = rmdir(folderName);
	assert(result == 0);

	for (int i = 0; i < fileCount; i++) {
		free(fileNames[i]);
		free(fileAliasNames[i]);
	}

	free(folderName);
	free(fileNames);
	free(fileAliasNames);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	test1(argv[0]);
	test2(argv[0]);
	test3(argv[0]);
	test4(argv[0]);
	test5(argv[0]);
	test6(argv[0]);
	test7(argv[0]);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
