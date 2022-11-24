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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

#include "util/math_utils.h"

static const int ZEROS_BEFORE = 49323;
static const char* const STRING = "The quick brown fox jumps over the lazy dog";

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
		fseek(file, ZEROS_BEFORE, SEEK_SET);
		assert(!feof(file));
		for (int i = 0; i < strlen(STRING); i++) {
			assert(ftell(file) == ZEROS_BEFORE + i);
			size_t result = fwrite(STRING + i, sizeof(char), sizeof(char), file);
			assert(result == sizeof(char));
		}
		fclose(file);
	}

	{
		 struct stat statInstance;
		 int result = stat(temporaryFileName, &statInstance);
		 assert(result == 0);
		 assert(ZEROS_BEFORE + strlen(STRING) == statInstance.st_size);
	}

	{
		FILE* file = fopen(temporaryFileName, "r");
		assert(file != NULL);
		int zeroCount = 0;
		char c;
		while ((c = fgetc(file)) == '\0') {
			zeroCount++;
		}
		assert(zeroCount == ZEROS_BEFORE);
		assert(c != EOF);
		int result = ungetc(c, file);
		assert(result == c);
		assert(ftell(file) == ZEROS_BEFORE);

		char* buffer = malloc(sizeof(char) * (strlen(STRING) + 1));
		assert(buffer != NULL);

		for (int i = 0; i < strlen(STRING); i++) {
			c = fgetc(file);
			assert(c != EOF);
			buffer[i] = c;
		}
		buffer[strlen(STRING)] = '\0';
		assert(strcmp(STRING, buffer) == 0);

		assert(!feof(file));
		c = fgetc(file);
		assert(c == EOF);
		assert(feof(file));

		fseek(file, ZEROS_BEFORE, SEEK_SET);
		assert(!feof(file));
		result = fread(buffer, sizeof(char), strlen(STRING), file);
		assert(result == strlen(STRING));
		assert(!feof(file));
		buffer[strlen(STRING)] = '\0';
		assert(strcmp(STRING, buffer) == 0);

		fclose(file);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
