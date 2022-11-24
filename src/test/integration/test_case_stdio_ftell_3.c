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

static const char* const FILE_CONTENT = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	assert(fileno(stdin) == STDIN_FILENO);
	assert(fileno(stdout) == STDOUT_FILENO);
	assert(fileno(stderr) == STDERR_FILENO);

	char* temporaryFileName = integrationTestCreateTemporaryFileName(argv[0]);
	assert(temporaryFileName != NULL);

	{
		FILE* file = fopen(temporaryFileName, "w");
		assert(file != NULL);
		for (int i = 0; i < strlen(FILE_CONTENT); i++) {
			assert(ftell(file) == i);
			size_t result = fwrite(FILE_CONTENT + i, sizeof(char), sizeof(char), file);
			assert(result == sizeof(char));
		}
		fclose(file);
	}

	{
		char* buffer = malloc(sizeof(char) * (strlen(FILE_CONTENT) + 1));
		assert(buffer != NULL);

		FILE* file = fopen(temporaryFileName, "r");
		assert(file != NULL);
		{
			int result = fseek(file, 0, SEEK_END);
			assert(result == 0);
		}

		long fileSize = ftell(file);
		assert(fileSize == strlen(FILE_CONTENT));

		rewind(file);

		{
			size_t result = fread(buffer, 1, strlen(FILE_CONTENT), file);
			assert(result == strlen(FILE_CONTENT));
			buffer[strlen(FILE_CONTENT)] = '\0';
		}
		assert(strcmp(FILE_CONTENT, buffer) == 0);

		fclose(file);

		{
			struct stat statInstance;
			int result = stat(temporaryFileName, &statInstance);
			assert(result == 0);
			assert(statInstance.st_size == fileSize);
		}

		free(buffer);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
