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
		char* buffer = malloc(sizeof(char) * (strlen(FILE_CONTENT) + 1));
		assert(buffer != NULL);

		FILE* file = fopen(temporaryFileName, "r");
		assert(file != NULL);
		int i = 0;
		char c;
		while ((c = fgetc(file)) != EOF) {
			assert(ftell(file) == i + 1);
			buffer[i] = c;

			if (i > 0 && i % 5 == 0) {
				c = ungetc(c, file);
				assert(c != EOF);
				assert(buffer[i] == c);

				assert(ftell(file) == i);
				c = fgetc(file);
				assert(buffer[i] == c);
			}

			i++;
		}
		buffer[i] = '\0';

		assert(strlen(buffer) == strlen(FILE_CONTENT));
		assert(strcmp(FILE_CONTENT, buffer) == 0);
		fclose(file);

		free(buffer);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
