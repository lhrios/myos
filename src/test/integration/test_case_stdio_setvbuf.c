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
	char* temporaryFileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(temporaryFileName != NULL);

	const char* content = "aaa BBB C";

	{
		size_t bufferSize = 128;
		char* buffer = malloc(sizeof(char) * bufferSize);
		assert(buffer != NULL);

		FILE* file = fopen(temporaryFileName, "w");
		assert(file != NULL);
		int result = setvbuf(file, buffer, _IOFBF, bufferSize);
		assert(result == 0);

		char* string;
		string = "aaa";
		fwrite(string, strlen(string), 1, file);

		string = " BBB";
		fwrite(string, strlen(string), 1, file);

		string = " C";
		fwrite(string, strlen(string), 1, file);

		assert(strncmp(content, buffer, strlen(content)) == 0);

		struct stat statInstance;
		result = fstat(fileno(file), &statInstance);
		assert(result == 0);
		assert(statInstance.st_size == 0);

		fclose(file);
		free(buffer);
	}

	char* fileContent = integrationReadFileContentAsString(temporaryFileName);
	assert(strcmp(content, fileContent) == 0);
	free(fileContent);
}

static void test2(const char* testCaseName) {
	char* temporaryFileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(temporaryFileName != NULL);

	const char* content = "111 222 333";

	{
		FILE* file = fopen(temporaryFileName, "w");
		assert(file != NULL);
		int result = setvbuf(file, NULL, _IOFBF, 128);
		assert(result == 0);

		char* string;
		string = "111";
		fwrite(string, strlen(string), 1, file);

		string = " 222";
		fwrite(string, strlen(string), 1, file);

		string = " 33";
		fwrite(string, strlen(string), 1, file);
		fputc('3', file);

		struct stat statInstance;
		result = fstat(fileno(file), &statInstance);
		assert(result == 0);
		assert(statInstance.st_size == 0);

		fclose(file);
	}

	char* fileContent = integrationReadFileContentAsString(temporaryFileName);
	assert(strcmp(content, fileContent) == 0);
	free(fileContent);
}

static void test3(const char* testCaseName) {
	char* temporaryFileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(temporaryFileName != NULL);

	const char* content = "XXX YYY ZZZ";

	{
		size_t bufferSize = 128;
		char* buffer = malloc(sizeof(char) * bufferSize);
		assert(buffer != NULL);

		FILE* file = fopen(temporaryFileName, "w");
		assert(file != NULL);
		setbuf(file, buffer);

		char* string;
		string = "XX";
		fwrite(string, strlen(string), 1, file);

		string = "X YYY";
		fwrite(string, strlen(string), 1, file);

		string = " ZZ";
		fwrite(string, strlen(string), 1, file);
		fputc('Z', file);

		assert(strncmp(content, buffer, strlen(content)) == 0);

		struct stat statInstance;
		int result = fstat(fileno(file), &statInstance);
		assert(result == 0);
		assert(statInstance.st_size == 0);

		fclose(file);
		free(buffer);
	}

	char* fileContent = integrationReadFileContentAsString(temporaryFileName);
	assert(strcmp(content, fileContent) == 0);
	free(fileContent);
}

static void test4(const char* testCaseName) {
	char* temporaryFileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(temporaryFileName != NULL);

	{
		size_t bufferSize = 128;
		char* buffer = malloc(sizeof(char) * bufferSize);
		assert(buffer != NULL);

		FILE* file = fopen(temporaryFileName, "w");
		assert(file != NULL);
		int result = setvbuf(file, buffer, _IOLBF, 128);
		assert(result == 0);

		char* string;
		string = "XX";
		fwrite(string, strlen(string), 1, file);

		string = "X\nYYY";
		fwrite(string, strlen(string), 1, file);

		string = " ZZ";
		fwrite(string, strlen(string), 1, file);
		fputc('Z', file);

		assert(strncmp("YYY ZZ", buffer, strlen("YYY ZZ")) == 0);

		struct stat statInstance;
		result = fstat(fileno(file), &statInstance);
		assert(result == 0);
		assert(statInstance.st_size == 4);
	}

	const char* CONTENT = "XXX\n";
	char* fileContent = integrationReadFileContentAsString(temporaryFileName);
	assert(strncmp(CONTENT, fileContent, strlen(CONTENT)) == 0);
	free(fileContent);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	test1(argv[0]);
	test2(argv[0]);
	test3(argv[0]);
	test4(argv[0]);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
