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

static void test1(const char* testCaseName) {
	const char* const FILE_CONTENT = "AAA\nBBBB\nCCCCCCCC\nDDD\n";
	char* temporaryFileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(temporaryFileName != NULL);

	const size_t bufferSize = sizeof(char) * 5;
	char* buffer = malloc(bufferSize);
	assert(buffer != NULL);

	FILE* file;
	{
		file = fopen(temporaryFileName, "w+");
		assert(file != NULL);
		size_t result = fwrite(FILE_CONTENT, sizeof(char), strlen(FILE_CONTENT), file);
		assert(result == strlen(FILE_CONTENT));
		rewind(file);

		char* fgetsResult = fgets(buffer, bufferSize, file);
		assert(fgetsResult == buffer);
		assert(strcmp(buffer, "AAA\n") == 0);

		fgetsResult = fgets(buffer, bufferSize, file);
		assert(fgetsResult == buffer);
		assert(strcmp(buffer, "BBBB") == 0);

		fgetsResult = fgets(buffer, bufferSize, file);
		assert(fgetsResult == buffer);
		assert(strcmp(buffer, "\n") == 0);

		fgetsResult = fgets(buffer, bufferSize, file);
		assert(fgetsResult == buffer);
		assert(strcmp(buffer, "CCCC") == 0);

		fgetsResult = fgets(buffer, bufferSize, file);
		assert(fgetsResult == buffer);
		assert(strcmp(buffer, "CCCC") == 0);

		fgetsResult = fgets(buffer, bufferSize, file);
		assert(fgetsResult == buffer);
		assert(strcmp(buffer, "\n") == 0);

		fgetsResult = fgets(buffer, bufferSize, file);
		assert(fgetsResult == buffer);
		assert(strcmp(buffer, "DDD\n") == 0);

		fgetsResult = fgets(buffer, bufferSize, file);
		assert(fgetsResult == NULL);
		assert(feof(file));

		fclose(file);
	}

	{
		int fileDescriptorIndex = open(temporaryFileName, O_WRONLY);
		assert(fileDescriptorIndex >= 0);

		file = fdopen(fileDescriptorIndex, "r");
		assert(file != NULL);
		assert(ftell(file) == 0);
		char* result = fgets(buffer, bufferSize, file);
		assert(result == NULL);
		assert(ferror(file));
		assert(!feof(file));
		fclose(file);
	}
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	assert(fileno(stdin) == STDIN_FILENO);
	assert(fileno(stdout) == STDOUT_FILENO);
	assert(fileno(stderr) == STDERR_FILENO);

	test1(argv[0]);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
