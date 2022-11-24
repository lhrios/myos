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
#include <ctype.h>
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

	char* temporaryFileName = integrationTestCreateTemporaryFileName(argv[0]);
	assert(temporaryFileName != NULL);

	const int fileContentLength = strlen(FILE_CONTENT);

	{
		FILE* file = fopen(temporaryFileName, "w");
		assert(file != NULL);
		size_t result = fwrite(FILE_CONTENT, sizeof(char), fileContentLength, file);
		assert(result == fileContentLength);
		fclose(file);
	}

	{
		FILE* file = fopen(temporaryFileName, "r+");
		assert(file != NULL);

		for (int i = 0; i < fileContentLength; i++) {
			assert(FILE_CONTENT[i] == fgetc(file));
		}
		assert(fgetc(file) == EOF);
		assert(feof(file));

		assert(fputs(FILE_CONTENT, file) >= 0);
		assert(feof(file));

		{
			const int bufferSize = 7;
			char* buffer = malloc(sizeof(char) * bufferSize);
			assert(buffer);
			int result = fseek(file, fileContentLength - 3, SEEK_SET);
			assert(result == 0);
			assert(fgets(buffer, bufferSize, file) != NULL);
			assert(strcmp(buffer, "dogThe") == 0);
		}
		assert(!feof(file));

		fseek(file, 0, SEEK_SET);
		for (int i = 0; i < fileContentLength / 2; i++) {
			fputc(toupper(FILE_CONTENT[i]), file);
		}
		fflush(file);

		{
			const int bufferSize = 10;
			char* buffer = malloc(sizeof(char) * bufferSize);
			assert(buffer);
			assert(fgets(buffer, bufferSize, file) != NULL);
			assert(strcmp(buffer, "umps over") == 0);
		}

		fclose(file);
	}

	{
		FILE* file = fopen(temporaryFileName, "r");
		assert(file != NULL);

		const int bufferSize = fileContentLength * 2;
		char* buffer = malloc(sizeof(char) * bufferSize);
		assert(buffer);

		size_t result = fread(buffer, sizeof(char), bufferSize, file);
		assert(result == bufferSize);
		assert(strncmp(buffer, "THE QUICK BROWN FOX Jumps over the lazy dog", fileContentLength) == 0);
		assert(strncmp(buffer + fileContentLength, FILE_CONTENT, fileContentLength) == 0);
		fclose(file);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
