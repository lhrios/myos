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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include "test/integration_test.h"

static void doChild(const char* fileName) {
	FILE* file = fopen(fileName, "w");
	assert(file != NULL);

	int result;

	result = fputc('|', file);
	assert(result == '|');

	result = fprintf(file, "ABC");
	assert(result == 3);

	result = fprintf(file, " %d %d", 123, 456);
	assert(result == 8);

	result = fputc('|', file);
	assert(result == '|');

	result = fputs("DEF|", file);
	assert(result >= 0);

	exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	char* fileName = integrationTestCreateTemporaryFileName(argv[0]);
	assert(fileName != NULL);

	pid_t childProcessId = fork();
	if (childProcessId == 0) {
		doChild(fileName);
	} else {
		assert(childProcessId > 0);
	}

	{
		int status;
		pid_t result = wait(&status);
		assert(result == childProcessId);
		assert(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);
	}

	const char* fileContent = "|ABC 123 456|DEF|";
	FILE* file = fopen(fileName, "r");

	const int bufferSize = 1024;
	char buffer[bufferSize];
	int result = fread(buffer, 1, bufferSize, file);
	assert(result == strlen(fileContent));
	assert(strncmp(fileContent, buffer, strlen(fileContent)) == 0);
	assert(fclose(file) == 0);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
