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

#include <sys/wait.h>

#include "test/integration_test.h"

static FILE* resultFile;
static volatile bool done = false;

static void handleSignal(int signalId) {
	if (signalId == SIGUSR2) {
		fprintf(resultFile, "Handled SIGUSR2\n");

	} else if (signalId == SIGINT) {
		fprintf(resultFile, "Handled SIGINT\n");
		done = true;

	} else {
		assert(false);
	}
}

static int doChild(const char* resultFileName) {
	resultFile = fopen(resultFileName, "w");
	assert(resultFile != NULL);

	while (!done) {
		sleep(5);
	}

	fclose(resultFile);

	return EXIT_SUCCESS;
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

	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGUSR2);
	result = sigaction(SIGINT, &action, NULL);
	assert(result == 0);

	action.sa_handler = SIG_IGN;
	result = sigaction(SIGHUP, &action, NULL);
	assert(result == 0);

	sigset_t signalSet;
	sigemptyset(&signalSet);
	sigaddset(&signalSet, SIGUSR1);
	result = sigprocmask(SIG_BLOCK, &signalSet, NULL);
	assert(result == 0);


	integrationTestConfigureCommonSignalHandlers();

	char* resultFileName = integrationTestCreateTemporaryFileName(argv[0]);

	pid_t childProcessId = fork();
	assert(childProcessId != (pid_t) - 1);
	if (childProcessId == 0) {
		return doChild(resultFileName);

	} else {
		while (!integrationTestFileExists(resultFileName)) {
			sleep(10);
		}

		kill(childProcessId, SIGINT);
		kill(childProcessId, SIGHUP);
		kill(childProcessId, SIGUSR1);
		kill(childProcessId, SIGUSR2);

		int status;
		pid_t processId = wait(&status);
		assert(processId == childProcessId);
		assert(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);

		const char* const EXPECTED_RESULT = "Handled SIGINT\nHandled SIGUSR2\n";
		char* actualFileContent = integrationReadFileContentAsString(resultFileName);
		assert(strcmp(EXPECTED_RESULT, actualFileContent) == 0);
		free(actualFileContent);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
