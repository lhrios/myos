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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include "test/integration_test.h"

static const char* const FILE_CONTENT_1 = "2gyL9nrrnv\ng3CgBDdFnk\nhf9OD1eTp6\n2E2y8eyV16";
static const char* const FILE_CONTENT_2 = "sN0Q06MN8a\ndsiN67ViA0\nAZH4ARyQn5\ndN5G0uwN0R\nV2yeyJsaE7";

static void createFile(const char* fileName, const char* fileContent) {
	FILE* file = fopen(fileName, "w");
	assert(file != NULL);
	int result = fwrite(fileContent, sizeof(char), strlen(fileContent), file);
	assert(result == strlen(fileContent));
	fclose(file);
}

static void verifyFileContent(const char* fileName, const char* expectedFileContent) {
	char* fileContent = integrationReadFileContentAsString(fileName);
	assert(strcmp(fileContent, expectedFileContent) == 0);
	free(fileContent);
}

static void test1(const char* testCaseName) {
	int result;
	char* oldFileName = integrationTestCreateTemporaryFileName(testCaseName);
	char* newFileName = integrationTestCreateTemporaryFileName(testCaseName);

	createFile(oldFileName, FILE_CONTENT_1);

	result = rename(oldFileName, newFileName);
	assert(result == 0);

	struct stat statInstance;
	result = stat(oldFileName, &statInstance);
	assert(result == -1);
	assert(errno == ENOENT);

	verifyFileContent(newFileName, FILE_CONTENT_1);
}

static void test2(const char* testCaseName) {
	int result;
	char* oldFileName = integrationTestCreateTemporaryFileName(testCaseName);
	char* newFileName = integrationTestCreateTemporaryFileName(testCaseName);

	createFile(oldFileName, FILE_CONTENT_2);
	createFile(newFileName, FILE_CONTENT_1);

	result = rename(oldFileName, newFileName);
	assert(result == 0);

	struct stat statInstance;
	result = stat(oldFileName, &statInstance);
	assert(result == -1);
	assert(errno == ENOENT);

	verifyFileContent(newFileName, FILE_CONTENT_2);
}

static void test3(const char* testCaseName) {
	int result;
	char* oldFolderName = integrationTestCreateTemporaryFileName(testCaseName);
	char* newFolderName = integrationTestCreateTemporaryFileName(testCaseName);

	result = mkdir(oldFolderName, 0);
	assert(result == 0);
	result = mkdir(newFolderName, 0);
	assert(result == 0);

	const char* oldCwd = getcwd(NULL, 0);
	assert(oldCwd != NULL);

	result = chdir(oldFolderName);
	assert(result == 0);

	createFile("file1", FILE_CONTENT_1);
	createFile("file2", FILE_CONTENT_2);

	result = chdir(oldCwd);
	assert(result == 0);

	result = rename(oldFolderName, newFolderName);
	assert(result == 0);

	struct stat statInstance;
	result = stat(oldFolderName, &statInstance);
	assert(result == -1);
	assert(errno == ENOENT);

	result = chdir(newFolderName);
	assert(result == 0);

	verifyFileContent("file1", FILE_CONTENT_1);
	verifyFileContent("file2", FILE_CONTENT_2);

	result = chdir(oldCwd);
	assert(result == 0);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	test1(argv[0]);
	test2(argv[0]);
	test3(argv[0]);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
