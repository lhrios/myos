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
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

#define PROCESS_GROUP_COUNT 5
#define CHILDREN_COUNT 20
#define PROCESSES_PER_GROUP (CHILDREN_COUNT / PROCESS_GROUP_COUNT)

static void handleSignal(int signalId) {
	assert(false);
}

static int doChild(int localId, pid_t processGroupId, char* informSetpgidDoneFileName) {
	setpgid(0, processGroupId);
	assert(getpgrp() == processGroupId);

	int result = unlink(informSetpgidDoneFileName);
	assert(result == 0);

	while (true) {
		sleep(5);
	}
	assert(false);

	return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	pid_t childrenProcessIds[CHILDREN_COUNT];
	char* informSetpgidDoneFileName[CHILDREN_COUNT];

	for (int localId = 0; localId < CHILDREN_COUNT; localId++) {
		char* fileName = integrationTestCreateTemporaryFileName(argv[0]);
		assert(fileName != NULL);
		int fileDescriptorIndex = open(fileName, O_TRUNC | O_WRONLY | O_CREAT);
		assert(fileDescriptorIndex >= 0);
		close(fileDescriptorIndex);
		informSetpgidDoneFileName[localId] = fileName;

		pid_t processId = fork();
		if (processId > 0) {
			childrenProcessIds[localId] = processId;
			pid_t processGroupId = childrenProcessIds[localId - (localId % PROCESSES_PER_GROUP)];
			setpgid(processId, processGroupId);

		} else if (processId == 0) {
			return doChild(localId, localId % PROCESSES_PER_GROUP == 0 ? getpid() : childrenProcessIds[localId - (localId % PROCESSES_PER_GROUP)], fileName);

		} else {
			assert(false);
		}
	}

	/* Enable SIGCHLD capture. */
	{
		struct sigaction action;

		memset(&action, 0, sizeof(struct sigaction));
		action.sa_handler = &handleSignal;
		sigemptyset(&action.sa_mask);
		int result = sigaction(SIGCHLD, &action, NULL);
		assert(result == 0);
	}

	/* Wait until all children are ready. */
	while (true) {
		sleep(1);

		int existentFileCount = 0;
		for (int i = 0; i < CHILDREN_COUNT; i++) {
			struct stat statInstance;
			int result = stat(informSetpgidDoneFileName[i], &statInstance);
			if (result == 0) {
				existentFileCount++;
			}
		}

		if (existentFileCount == 0) {
			break;
		}
	}

	/* Disable SIGCHLD capture. */
	{
		struct sigaction action;

		memset(&action, 0, sizeof(struct sigaction));
		action.sa_handler = SIG_IGN;
		sigemptyset(&action.sa_mask);
		int result = sigaction(SIGCHLD, &action, NULL);
		assert(result == 0);
	}

	int processGroupsSignals[PROCESS_GROUP_COUNT] = {
		SIGUSR1,
		SIGUSR2,
		SIGKILL,
		SIGINT,
		SIGQUIT
	};
	for (int i = 0; i < PROCESS_GROUP_COUNT; i++) {
		pid_t processGroupId = childrenProcessIds[i * PROCESSES_PER_GROUP];
		kill(-processGroupId, processGroupsSignals[i]);
	}

	int processCount = 0;
	while (processCount < CHILDREN_COUNT) {
		int status;
		pid_t processId = wait(&status);
		if (processId > 0) {
			for (int i = 0; i < CHILDREN_COUNT; i++) {
				assert(WIFSIGNALED(status));
				if (childrenProcessIds[i] == processId) {
					assert(WTERMSIG(status) == processGroupsSignals[i / PROCESSES_PER_GROUP]);
					processCount++;
					break;
				}
			}
		}
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
