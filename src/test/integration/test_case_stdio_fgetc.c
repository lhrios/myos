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

static const char* const FILE_CONTENT = "ABC123";

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

	int character;
	character = getchar();
	assert(character == 'A');

	character = getchar();
	assert(character == 'B');

	character = getchar();
	assert(character == 'C');

	character = getchar();
	assert(character == '1');

	character = getchar();
	assert(character == '2');

	character = getchar();
	assert(character == '3');

	character = getchar();
	assert(character == EOF);
	assert(feof(stdin));

	character = getchar();
	assert(character == EOF);
	assert(feof(stdin));

	fprintf(stdin, "xyz\n");
	fflush(stdin);
	assert(ferror(stdin));

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
