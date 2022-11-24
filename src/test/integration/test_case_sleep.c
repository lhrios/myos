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
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include "test/integration_test.h"

static pid_t spawnChild(int secondsToSleep) {
	pid_t result = fork();
	if (result == 0) {
		char* argv[2];

		char buffer1[64];
		sprintf(buffer1, "%d", secondsToSleep);
		argv[0] = buffer1;
		argv[1] = NULL;

		execvp(INTEGRATION_TEST_EXECUTABLES_PATH "executable_sleep", argv);

	} else if (result > 0) {
		return result;
	}
	assert(false);
	exit(EXIT_FAILURE);
	return result;
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	const int childrenCount = 5;
	pid_t childrenIds[childrenCount];

	for (int i = childrenCount - 1; i >= 0; i--) {
		childrenIds[i] = spawnChild(15 * i);
	}

	int terminatedChildrenCount = 0;
	while (true) {
		int status;
		pid_t result = waitpid(-1, &status, 0);
		if (result != -1) {
			assert(childrenIds[terminatedChildrenCount++] == result);
			assert(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);
		} else {
			assert(errno == ECHILD);
			break;
		}
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
