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
#include <errno.h>
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

#define NUMBER_OF_CHARS_PER_WRITER 4096

static void doReader(int sequential, const char* outputDirectoryPath, int readFileDescriptorIndex, int writeFileDescriptorIndex) {
	close(writeFileDescriptorIndex);

	char* outputFilePath = malloc(sizeof(char) * (strlen(outputDirectoryPath) + 32));
	sprintf(outputFilePath, "%s/output%d.txt", outputDirectoryPath, sequential);
	int outputFileDescriptorIndex = open(outputFilePath, O_CREAT | O_WRONLY);
	assert(outputFileDescriptorIndex >= 0);
	free(outputFilePath);

	while (true) {
		const size_t bufferSize = 8;
		char buffer[bufferSize];
		ssize_t result = read(readFileDescriptorIndex, buffer, bufferSize);
		if (result < 0) {
			perror(NULL);
			assert(false);
		} else if (result == 0) {
			break;
		} else {
			ssize_t result2 = write(outputFileDescriptorIndex, buffer, result);
			assert(result == result2);
		}

		if (rand() % 500 == 1) {
			sleep(6);
		}
	}

	exit(EXIT_SUCCESS);
}

static void doWriter(int sequential, const char* outputDirectoryPath, int readFileDescriptorIndex, int writeFileDescriptorIndex) {
	close(readFileDescriptorIndex);

	sleep(5);

	char character = 'A' + sequential;
	for (int i = 0; i < NUMBER_OF_CHARS_PER_WRITER; i++) {
		ssize_t result = write(writeFileDescriptorIndex, &character, sizeof(char));
		assert(result == 1);

		if (rand() % 500 == 1) {
			sleep(3);
		}
	}

	exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	int pipeFileDescriptorIndexes[2];
	{
		int result = pipe(pipeFileDescriptorIndexes);
		assert(result == 0);
	}

	char* outputDirectoryPath = malloc(sizeof(char) * 128);
	assert(outputDirectoryPath != NULL);
	{
		sprintf(outputDirectoryPath, "/tmp/%s", argv[0]);
		int result = mkdir(outputDirectoryPath, 0);
		assert(result == 0);
	}

	int childProcessCount = 0;

	const int readerProcessCount = 4;
	int readerProcessId[readerProcessCount];
	for (int i = 0; i < readerProcessCount; i++) {
		pid_t childProcessId = fork();
		if (childProcessId == 0) {
			doReader(i, outputDirectoryPath, pipeFileDescriptorIndexes[0], pipeFileDescriptorIndexes[1]);
		} else {
			readerProcessId[i] = childProcessId;
			childProcessCount++;
			assert(childProcessId > 0);
		}
	}

	const int writerProcessCount = 6;
	int writerProcessId[writerProcessCount];
	for (int i = 0; i < writerProcessCount; i++) {
		pid_t childProcessId = fork();
		if (childProcessId == 0) {
			doWriter(i, outputDirectoryPath, pipeFileDescriptorIndexes[0], pipeFileDescriptorIndexes[1]);
		} else {
			writerProcessId[i] = childProcessId;
			childProcessCount++;
			assert(childProcessId > 0);
		}
	}

	close(pipeFileDescriptorIndexes[0]);
	close(pipeFileDescriptorIndexes[1]);

	while (childProcessCount >= 0) {
		int status;
		pid_t result = waitpid(-1, &status, 0);
		if (result != -1) {
			bool found = false;
			for (int i = 0; i < readerProcessCount; i++) {
				found = found || result == readerProcessId[i];
			}
			for (int i = 0; i < writerProcessCount; i++) {
				found = found || result == writerProcessId[i];
			}

			assert(found);
			assert(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);
		} else {
			assert(errno == ECHILD);
			break;
		}
	}

	{
		int countByWriterProcess[writerProcessCount];
		memset(countByWriterProcess, 0, sizeof(int) * writerProcessCount);

		for (int i = 0; i < readerProcessCount; i++) {
			char* outputFilePath = malloc(sizeof(char) * (strlen(outputDirectoryPath) + 32));
			sprintf(outputFilePath, "%s/output%d.txt", outputDirectoryPath, i);
			int outputFileDescriptorIndex = open(outputFilePath, O_RDONLY);
			assert(outputFileDescriptorIndex >= 0);
			free(outputFilePath);

			while (true) {
				const size_t bufferSize = 8;
				char buffer[bufferSize];
				ssize_t result = read(outputFileDescriptorIndex, buffer, bufferSize);
				if (result < 0) {
					perror(NULL);
					assert(false);
				} else if (result == 0) {
					break;
				} else {
					for (int j = 0; j < result; j++) {
						char character = buffer[j];
						assert(character >= 'A' && character - 'A' < writerProcessCount);
						countByWriterProcess[character - 'A']++;
					}
				}
			}

			close(outputFileDescriptorIndex);
		}

		for (int i = 0; i < writerProcessCount; i++) {
			assert(countByWriterProcess[i] == NUMBER_OF_CHARS_PER_WRITER);
		}
	}

	free(outputDirectoryPath);

	integrationTestRegisterSuccessfulCompletion(argv[0]);

	return EXIT_SUCCESS;
}
