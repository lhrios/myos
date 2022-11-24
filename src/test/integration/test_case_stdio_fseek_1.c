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

static const char* const ORIGINAL_FILE_CONTENT = "The quick brown fox jumps over the lazy dog";
static const char* const NEW_FILE_CONTENT = "The quick black fox jumps over the fast dog";

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
		for (int i = 0; i < strlen(ORIGINAL_FILE_CONTENT); i++) {
			assert(ftell(file) == i);
			size_t result = fwrite(ORIGINAL_FILE_CONTENT + i, sizeof(char), sizeof(char), file);
			assert(result == sizeof(char));
		}
		fclose(file);
	}

	{
		char* buffer = malloc(sizeof(char) * (strlen(NEW_FILE_CONTENT) + 1));
		assert(buffer != NULL);

		FILE* file = fopen(temporaryFileName, "r+");
		assert(file != NULL);

		assert(fseek(file, 10, SEEK_SET) != -1);
		size_t result = fread(buffer, sizeof(char), sizeof(char) * 5, file);
		assert(result == 5);
		buffer[5] = '\0';
		assert(strcmp(buffer, "brown") == 0);

		assert(fseek(file, -5, SEEK_CUR) != -1);
		result = fwrite("black", sizeof(char), sizeof(char) * 5, file);
		assert(result == 5);

		assert(fseek(file, -8, SEEK_END) != -1);
		result = fread(buffer, sizeof(char), sizeof(char) * 4, file);
		assert(result == 4);
		buffer[4] = '\0';
		assert(strcmp(buffer, "lazy") == 0);

		assert(fseek(file, -4, SEEK_CUR) != -1);
		result = fwrite("fast", sizeof(char), sizeof(char) * 4, file);
		assert(result == 4);

		rewind(file);
		result = fread(buffer, sizeof(char), UINT_MAX, file);
		assert(result == strlen(NEW_FILE_CONTENT));
		buffer[result] = '\0';
		assert(strcmp(buffer, NEW_FILE_CONTENT) == 0);

		result = fgetc(file);
		assert(result == EOF);
		assert(feof(file));

		fclose(file);

		free(buffer);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
