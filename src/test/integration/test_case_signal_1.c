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
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <myos.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

#include "util/math_utils.h"

static int nextEventTime = 1;
static int eventTimeSIGUSR1 = 0;
static int eventTimeSIGUSR2 = 0;

static void handleSignal(int signalId) {
	if (signalId == SIGUSR1) {
		eventTimeSIGUSR1 = nextEventTime++;

	} else if (signalId == SIGUSR2) {
		eventTimeSIGUSR2 = nextEventTime++;

	} else {
		assert(false);
	}
}

static void doChild(const char* testCaseName) {
	int result;
	struct sigaction action;

	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &handleSignal;
	sigemptyset(&action.sa_mask);
	result = sigaction(SIGUSR1, &action, NULL);
	assert(result == 0);

	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &handleSignal;
	sigemptyset(&action.sa_mask);
	result = sigaction(SIGUSR2, &action, NULL);
	assert(result == 0);

	raise(SIGSTOP);

	sleep(5);

	assert(eventTimeSIGUSR1 == 1 && eventTimeSIGUSR2 == 2);
	exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	pid_t childProcessId = fork();
	if (childProcessId == 0) {
		doChild(argv[0]);

	} else if (childProcessId > 0) {
		int status;
		pid_t result;
		int count = 0;
		while ((result = waitpid(-1, &status, WUNTRACED | WCONTINUED)) != -1) {
			if (WIFSTOPPED(status)) {
				kill(childProcessId, SIGUSR1);
				kill(childProcessId, SIGUSR2);
				kill(childProcessId, SIGCONT);
				count++;

			} else if (WIFCONTINUED(status)) {
				count++;

			} else {
				assert(WIFEXITED(status));
				assert(WEXITSTATUS(status) == EXIT_SUCCESS);
				count++;
			}
		}
		assert(count == 3);

	} else {
		assert(false);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
