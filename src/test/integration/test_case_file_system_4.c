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
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include <myos.h>

#include "util/math_utils.h"

#include "test/integration_test.h"

static const char* const EXPECTED_FILE_CONTENT = "Mussum Ipsum, cacilds vidis litro abertis. "
		"Cevadis im ampola pa arma uma pindureta. Praesent vel viverra nisi. "
		"Mauris aliquet nunc non turpis scelerisque, eget. Em pé sem cair, deitado sem dormir, sentado sem cochilar e fazendo pose. "
		"Atirei o pau no gatis, per gatis num morreus.";

static void test1(const char* testCaseName) {
	char* fileName = integrationTestCreateTemporaryFileName(testCaseName);
	assert(fileName != NULL);

	size_t CONTENT_LENGTH = strlen(EXPECTED_FILE_CONTENT);
	const size_t BUFFER_SIZE = 67; /* So it won't align with block boundaries. */
	const int REPETITION_COUNT = 100;

	char* buffer = malloc(sizeof(char) * BUFFER_SIZE);
	assert(buffer != NULL);
	int result;

	{
		int fileDescriptorIndex = open(fileName, O_CREAT | O_WRONLY);
		assert(fileDescriptorIndex >= 0);

		for (int i = 0; i < REPETITION_COUNT; i++) {
			size_t totalCount = 0;
			while (totalCount < CONTENT_LENGTH) {
				size_t count = mathUtilsMin(BUFFER_SIZE, CONTENT_LENGTH - totalCount);
				memcpy(buffer, EXPECTED_FILE_CONTENT + totalCount, count);
				result = write(fileDescriptorIndex, buffer, count);
				assert (result == count);

				totalCount += count;
			}
		}

		result = close(fileDescriptorIndex);
		assert(result == 0);

	}

	{
		int fileDescriptorIndex = open(fileName, O_CREAT | O_RDONLY);
		assert(fileDescriptorIndex >= 0);

		char* content = malloc(sizeof(char) * CONTENT_LENGTH);
		assert(content != NULL);

		for (int i = 0; i < REPETITION_COUNT; i++) {
			size_t totalCount = 0;
			while (totalCount < CONTENT_LENGTH) {
				size_t count = mathUtilsMin(BUFFER_SIZE, CONTENT_LENGTH - totalCount);
				result = read(fileDescriptorIndex, buffer, count);
				assert (result == count);
				memcpy(content + totalCount, buffer, count);

				totalCount += count;
			}

			assert(strncmp(EXPECTED_FILE_CONTENT, content, CONTENT_LENGTH) == 0);
		}

		free(content);

		result = close(fileDescriptorIndex);
		assert(result == 0);
	}

	free(buffer);
	free(fileName);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	test1(argv[0]);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
