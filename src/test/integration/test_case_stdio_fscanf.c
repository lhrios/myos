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

#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

#include "util/math_utils.h"

static const char* const FILE_CONTENT = " A 1\nB    2\nC 3 ";

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	assert(fileno(stdin) == STDIN_FILENO);
	assert(fileno(stdout) == STDOUT_FILENO);
	assert(fileno(stderr) == STDERR_FILENO);

	char* temporaryFileName = integrationTestCreateTemporaryFileName(argv[0]);
	assert(temporaryFileName != NULL);

	int fileDescriptorIndex = open(temporaryFileName, O_CREAT | O_WRONLY);
	assert(fileDescriptorIndex >= 0);
	ssize_t result = write(fileDescriptorIndex, FILE_CONTENT, strlen(FILE_CONTENT));
	assert(result == strlen(FILE_CONTENT));
	{
		int result = close(fileDescriptorIndex);
		assert(result == 0);
	}

	fileDescriptorIndex = open(temporaryFileName, O_RDONLY);
	free(temporaryFileName);
	assert(fileDescriptorIndex >= 0);
	dup2(fileDescriptorIndex, STDIN_FILENO);

	{
		int result;
		char* string;
		int integer;

		result = scanf("%ms %d", &string, &integer);
		assert(result == 2);
		assert(strcmp(string, "A") == 0);
		assert(integer == 1);
		assert(!feof(stdin));
		assert(!ferror(stdin));
		free(string);

		result = scanf("%ms %d", &string, &integer);
		assert(result == 2);
		assert(strcmp(string, "B") == 0);
		assert(integer == 2);
		assert(!feof(stdin));
		assert(!ferror(stdin));
		free(string);

		result = scanf("%ms %d", &string, &integer);
		assert(result == 2);
		assert(strcmp(string, "C") == 0);
		assert(integer == 3);
		assert(!feof(stdin));
		assert(!ferror(stdin));
		free(string);

		result = scanf("%ms %d", &string, &integer);
		assert(result == EOF);
		assert(feof(stdin));
		assert(!ferror(stdin));
		free(string);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
