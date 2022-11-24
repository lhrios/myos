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

static const char* const FILE_CONTENT = "The quick brown fox jumps over the lazy dog";

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

			if (i % 5 == 0) {
				fflush(file);
			}
		}
		fclose(file);
	}

	{
		FILE* file = fopen(temporaryFileName, "a");
		assert(file != NULL);

		int result = ftell(file);
		assert(result == strlen(FILE_CONTENT));

		rewind(file);
		fputs(FILE_CONTENT, file);

		result = ftell(file);
		assert(result == 2 * strlen(FILE_CONTENT));
		fclose(file);
	}

	{
		char* actualFileContent = integrationReadFileContentAsString(temporaryFileName);
		assert(strlen(actualFileContent) == 2 * strlen(FILE_CONTENT));
		assert(strncmp(actualFileContent, FILE_CONTENT, strlen(FILE_CONTENT)) == 0);
		assert(strncmp(actualFileContent + strlen(FILE_CONTENT), FILE_CONTENT, strlen(FILE_CONTENT)) == 0);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
