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

/*
 * References:
 * - https://burtleburtle.net/bob/hash/integer.html
 * - https://web.archive.org/web/20080702045857/http://www.cris.com/~Ttwang/tech/inthash.htm
 */
static uint32_t hash(uint32_t value) {
    value = (value ^ 61) ^ (value >> 16);
    value = value + (value << 3);
    value = value ^ (value >> 4);
    value = value * 0x27d4eb2d;
    value = value ^ (value >> 15);
    return value;
}

static void writeToFile(int fileDescriptorIndex, uint32_t firstByteIndex, uint32_t bytesToWrite, size_t bufferSize) {
	assert(firstByteIndex % bufferSize == 0);
	assert((firstByteIndex + bytesToWrite) % bufferSize == 0);

	uint8_t* buffer = malloc(sizeof(uint8_t) * bufferSize);
	assert(buffer != NULL);

	for (uint32_t byteIndex = firstByteIndex; byteIndex < firstByteIndex + bytesToWrite; byteIndex++) {
		buffer[byteIndex % bufferSize] = hash(byteIndex) & 0xFF;

		if ((byteIndex % bufferSize) + 1 == bufferSize) {
			ssize_t result = write(fileDescriptorIndex, buffer, bufferSize);
			assert(result == bufferSize);
		}
	}

	free(buffer);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	char* fileName = integrationTestCreateTemporaryFileName(argv[0]);
	assert(fileName != NULL);

	const int bytesToWriteOnFirstStep = 1024 * 1024 * 16;
	{
		int fileDescriptorIndex = open(fileName, O_CREAT | O_WRONLY);
		assert(fileDescriptorIndex >= 0);

		writeToFile(fileDescriptorIndex, 0, bytesToWriteOnFirstStep, 256);

		close(fileDescriptorIndex);
	}

	const int bytesToWriteOnSecondStep = 1024 * 1024 * 32;
	{
		int fileDescriptorIndex = open(fileName, O_CREAT | O_WRONLY);
		assert(fileDescriptorIndex >= 0);

		off_t result = lseek(fileDescriptorIndex, bytesToWriteOnFirstStep, SEEK_SET);
		assert(result == bytesToWriteOnFirstStep);

		assert(fileDescriptorIndex >= 0);
		writeToFile(fileDescriptorIndex, bytesToWriteOnFirstStep, bytesToWriteOnSecondStep, 512);

		close(fileDescriptorIndex);
	}


	const int bytesToWriteOnThirdStep = 1024 * 1024 * 64;
	{
		int fileDescriptorIndex = open(fileName, O_CREAT | O_WRONLY);
		assert(fileDescriptorIndex >= 0);

		off_t result = lseek(fileDescriptorIndex, 0, SEEK_END);
		assert(result == bytesToWriteOnFirstStep + bytesToWriteOnSecondStep);

		assert(fileDescriptorIndex >= 0);
		writeToFile(fileDescriptorIndex, bytesToWriteOnFirstStep + bytesToWriteOnSecondStep, bytesToWriteOnThirdStep, 1024);

		close(fileDescriptorIndex);
	}

	sleep(120);

	{
		const size_t bufferSize = 2048;
		uint8_t* buffer = malloc(sizeof(uint8_t) * bufferSize);
		assert(buffer != NULL);

		int fileDescriptorIndex = open(fileName, O_RDONLY);
		assert(fileDescriptorIndex >= 0);

		size_t fileSize = bytesToWriteOnFirstStep + bytesToWriteOnSecondStep + bytesToWriteOnThirdStep;

		off_t result = lseek(fileDescriptorIndex, -((off_t) fileSize), SEEK_END);
		assert(result == 0);

		for (uint32_t i = 0; i < fileSize; i++) {
			if (i % bufferSize == 0) {
				ssize_t result = read(fileDescriptorIndex, buffer, bufferSize);
				assert(result == bufferSize);
			}

			assert((hash(i) & 0xFF) == buffer[i % bufferSize]);
		}
		close(fileDescriptorIndex);

		free(buffer);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
