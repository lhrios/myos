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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <myos.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

#include "util/math_utils.h"

static const char* const MESSAGE1 = "THEPIPEIMPLEMENTATIONSEEMSTOBEWORKINGPROPERLY";
static const char* const MESSAGE2 = "thepipeimplementationseemstobeworkingproperly";

static void doReader(const char* testCaseName, int readFileDescriptorIndex, int writeFileDescriptorIndex) {
	close(writeFileDescriptorIndex);

	const size_t bufferSize = 128;
	char buffer[bufferSize];
	char* bufferTop = buffer;

	while (true) {
		ssize_t result = read(readFileDescriptorIndex, bufferTop, bufferSize);
		if (result < 0) {
			perror(NULL);
			assert(false);
		} else if (result == 0) {
			break;
		} else {
			bufferTop += result;
		}
	}

	char message1[bufferSize];
	char message2[bufferSize];

	{
		char* message1Top = message1;
		char* message2Top = message2;

		for (int i = 0; i < (bufferTop - buffer); i++) {
			char c = buffer[i];
			if (islower(c)) {
				*message1Top = c;
				message1Top++;
			} else {
				*message2Top = c;
				message2Top++;
			}
		}

		*message1Top = '\0';
		*message2Top = '\0';
	}

	assert((strcmp(message1, MESSAGE1) == 0 && strcmp(message2, MESSAGE2) == 0)
			|| (strcmp(message2, MESSAGE1) == 0 && strcmp(message1, MESSAGE2) == 0));

	exit(EXIT_SUCCESS);
}

static void doWriter(const char* testCaseName, const char* message, int readFileDescriptorIndex, int writeFileDescriptorIndex) {
	close(readFileDescriptorIndex);

	ssize_t offset = 0;
	size_t available = strlen(message);
	while (true) {
		ssize_t result = write(writeFileDescriptorIndex, message + offset, mathUtilsMin(available, 7));
		if (result < 0) {
			perror(NULL);
			assert(false);
			break;
		} else if (result == 0) {
			break;
		} else {
			offset += result;
			available -= result;
		}
		sleep(1);
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

	pid_t child1ProcessId = fork();
	if (child1ProcessId == 0) {
		doReader(argv[0], pipeFileDescriptorIndexes[0], pipeFileDescriptorIndexes[1]);
	} else {
		assert(child1ProcessId > 0);
	}

	pid_t child2ProcessId = fork();
	if (child2ProcessId == 0) {
		doWriter(argv[0], MESSAGE1, pipeFileDescriptorIndexes[0], pipeFileDescriptorIndexes[1]);
	} else {
		assert(child2ProcessId > 0);
	}

	pid_t child3ProcessId = fork();
	if (child3ProcessId == 0) {
		doWriter(argv[0], MESSAGE2, pipeFileDescriptorIndexes[0], pipeFileDescriptorIndexes[1]);
	} else {
		assert(child3ProcessId > 0);
	}

	close(pipeFileDescriptorIndexes[0]);
	close(pipeFileDescriptorIndexes[1]);

	while (true) {
		int status;
		pid_t result = waitpid(-1, &status, 0);
		if (result != -1) {
			assert(child1ProcessId == result || child2ProcessId == result || child3ProcessId == result);
			assert(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);
		} else {
			assert(errno == ECHILD);
			break;
		}
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
