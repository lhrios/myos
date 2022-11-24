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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

static void doGrandChild(pid_t parentProcessId, pid_t grandFatherProcessId) {
	assert(parentProcessId != grandFatherProcessId);
	do {
		sleep(5);
	} while (kill(parentProcessId, 0) == 0);
	kill(grandFatherProcessId, SIGCONT);
	exit(EXIT_SUCCESS);
}

static void doChild(void) {
	pid_t parentProcessId = getppid();
	pid_t processId = getpid();

	int result = kill(parentProcessId, SIGSTOP);
	assert(result == 0);
	pid_t childProcessId = fork();
	if (childProcessId == 0) {
		doGrandChild(processId, parentProcessId);

	} else if (childProcessId > 0) {
		exit(EXIT_SUCCESS);

	} else {
		assert(false);
	}

	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	pid_t childProcessId = fork();
	if (childProcessId == 0) {
		doChild();

	} else if (childProcessId > 0) {
		int status;
		pid_t processId = wait(&status);
		assert(processId == childProcessId);

	} else {
		assert(false);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
