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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <myos.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

#include "util/math_utils.h"

static const char* const MESSAGE = "The pipe implementation seems to be working properly!!!";

static void handleSignal(int signalId) {
	exit(EXIT_SUCCESS);
}

static void doReader(int readFileDescriptorIndex, int writeFileDescriptorIndex) {
	sleep(5);
	exit(EXIT_SUCCESS);
}

static void doWriter(int readFileDescriptorIndex, int writeFileDescriptorIndex) {
	close(readFileDescriptorIndex);

	struct sigaction act;
	act.sa_flags = 0;
	act.sa_handler = &handleSignal;
	sigemptyset(&act.sa_mask);
	act.sa_sigaction = NULL;

	sigaction(SIGPIPE, &act, NULL);

	ssize_t offset = 0;
	size_t available = strlen(MESSAGE) + 1;
	while (true) {
		ssize_t result = write(writeFileDescriptorIndex, MESSAGE + offset, mathUtilsMin(available, 9));
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

	assert(false);

	exit(EXIT_FAILURE);
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
		doReader(pipeFileDescriptorIndexes[0], pipeFileDescriptorIndexes[1]);
	} else {
		assert(child1ProcessId > 0);
	}

	pid_t child2ProcessId = fork();
	if (child2ProcessId == 0) {
		doWriter(pipeFileDescriptorIndexes[0], pipeFileDescriptorIndexes[1]);
	} else {
		assert(child2ProcessId > 0);
	}

	close(pipeFileDescriptorIndexes[0]);
	close(pipeFileDescriptorIndexes[1]);

	while (true) {
		int status;
		pid_t result = waitpid(-1, &status, 0);
		if (result != -1) {
			assert(child1ProcessId == result || child2ProcessId == result);
			assert(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);
		} else {
			assert(errno == ECHILD);
			break;
		}
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
