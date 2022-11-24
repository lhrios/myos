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

#define CHUNK_SIZE 1024
#define WRITER_PROCESS_COUNT 8
#define FILE_SIZE (1024 * 1024 * 8)

static int doWriter(int writerId, const char* fileName) {
	int fileDescriptorIndex = open(fileName, O_WRONLY);
	assert(fileDescriptorIndex >= 0);

	uint8_t* chunk = malloc(sizeof(uint8_t) * CHUNK_SIZE);
	assert(chunk != NULL);

	for(off_t fileOffset = writerId * CHUNK_SIZE; fileOffset < FILE_SIZE; fileOffset += CHUNK_SIZE) {
		for (off_t chunkOffset = 0; chunkOffset < CHUNK_SIZE; chunkOffset++) {
			chunk[chunkOffset] = hash(fileOffset + chunkOffset) % 256;
		}

		off_t seekResult = lseek(fileDescriptorIndex, fileOffset, SEEK_SET);
		assert(seekResult == fileOffset);

		int result = write(fileDescriptorIndex, chunk, CHUNK_SIZE);
		assert(result == CHUNK_SIZE);
	}

	free(chunk);
	close(fileDescriptorIndex);

	return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	char* fileName = integrationTestCreateTemporaryFileName(argv[0]);
	assert(fileName != NULL);

	{
		int fileDescriptorIndex = open(fileName, O_CREAT | O_WRONLY);
		assert(fileDescriptorIndex >= 0);

		uint8_t* chunk = malloc(sizeof(uint8_t) * CHUNK_SIZE);
		assert(chunk != NULL);
		memset(chunk, 0, CHUNK_SIZE);

		int result = write(fileDescriptorIndex, chunk, CHUNK_SIZE);
		assert(result == CHUNK_SIZE);

		free(chunk);
		close(fileDescriptorIndex);
	}

	for(int writerId = 0; writerId < WRITER_PROCESS_COUNT; writerId++) {
		pid_t processId = fork();
		if (processId == 0) {
			return doWriter(writerId, fileName);
		} else if (processId < 0) {
			assert(false);
		}
	}

	int countOfChildrenFinished = 0;
	while (countOfChildrenFinished < WRITER_PROCESS_COUNT) {
		int status;
		wait(&status);
		assert(WIFEXITED(status));
		assert(WEXITSTATUS(status) == EXIT_SUCCESS);
		countOfChildrenFinished++;
	}

	{
		int fileDescriptorIndex = open(fileName, O_RDONLY);
		assert(fileDescriptorIndex >= 0);

		uint8_t* chunk = malloc(sizeof(uint8_t) * CHUNK_SIZE);
		assert(chunk != NULL);

		for(off_t fileOffset = 0; fileOffset < FILE_SIZE; fileOffset += CHUNK_SIZE) {
			int result = read(fileDescriptorIndex, chunk, CHUNK_SIZE);
			assert(result == CHUNK_SIZE);

			for (off_t chunkOffset = 0; chunkOffset < CHUNK_SIZE; chunkOffset++) {
				assert(chunk[chunkOffset] == hash(fileOffset + chunkOffset) % 256);
			}
		}

		free(chunk);
		close(fileDescriptorIndex);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
