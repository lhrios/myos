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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <myos.h>

#include <sys/wait.h>

#include "test/integration_test.h"

static void handleSignal(int signalId) {
	assert(signalId == SIGUSR2);
	exit(EXIT_SUCCESS);
}

static int doChild() {
	return EXIT_FAILURE;
}

int main(int argc, char** argv) {
	struct sigaction action;
	action.sa_handler = &handleSignal;
	action.sa_sigaction = NULL;
	action.sa_flags = 0;

	int result;
	sigemptyset(&action.sa_mask);
	result = sigaction(SIGUSR2, &action, NULL);
	assert(result == 0);

	integrationTestConfigureCommonSignalHandlers();

	pid_t childProcessId = myosForkAndGenerateSignal(SIGUSR2);
	assert(childProcessId != (pid_t) - 1);
	if (childProcessId == 0) {
		return doChild();

	} else {
		int status;
		pid_t processId = wait(&status);
		assert(processId == childProcessId);
		assert(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
