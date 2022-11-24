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
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include <myos.h>

#include "test/integration_test.h"

const char* const GRAND_CHILD_READY_FILE_NAME_FORMAT = "grandchild_%d_ready.txt";
const char* const GRAND_CHILD_RESULT_FILE_NAME_FORMAT = "grandchild_%d_result.txt";
const char* const GRAND_CHILD_RESULT_FILE_CONTENT_FORMAT = "Grandchild %d received signal %s\n";
const int DEFAULT_BUFFER_SIZE = 1024;
const int DEFAULT_SLEEP_INTERVAL = 5;
const int GRAND_CHILDREN_COUNT = 6;

static int globalGrandchildId = 0;

static void handleSignal(int signalId) {
	char* fileName = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
	assert(fileName != NULL);
	sprintf(fileName, GRAND_CHILD_RESULT_FILE_NAME_FORMAT, globalGrandchildId);

	FILE* resultFile = fopen(fileName, "w");
	assert(resultFile != NULL);
	fprintf(resultFile, GRAND_CHILD_RESULT_FILE_CONTENT_FORMAT, globalGrandchildId, strsignal(signalId));
	fclose(resultFile);
	exit(EXIT_SUCCESS);
}

static int doGrandchild(int stopSignalId, bool grandchildOnItsOwnProcessGroup) {
	int result;
	if (grandchildOnItsOwnProcessGroup) {
		result = setpgrp();
		assert(result == 0);
		assert(getpid() == getpgrp());
	}

	struct sigaction action;

	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &handleSignal;
	sigemptyset(&action.sa_mask);
	result = sigaction(SIGHUP, &action, NULL);
	assert(result == 0);

	char* fileName = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
	assert(fileName != NULL);
	sprintf(fileName, GRAND_CHILD_READY_FILE_NAME_FORMAT, globalGrandchildId);
	FILE* readyFile = fopen(fileName, "w");
	fclose(readyFile);

	raise(stopSignalId);

	while (true) {
		sleep(DEFAULT_SLEEP_INTERVAL);
	}

	return EXIT_FAILURE;
}

static int doChild(bool grandchildOnItsOwnProcessGroup) {
	int result = setpgrp();
	assert(result == 0);
	assert(getpid() == getpgrp());

	for (int grandchildId = 1; grandchildId <= GRAND_CHILDREN_COUNT; grandchildId++) {
		pid_t grandchildProcessId = fork();
		assert(grandchildProcessId != (pid_t) - 1);
		if (grandchildProcessId == 0) {
			globalGrandchildId = grandchildId;
			return doGrandchild(SIGSTOP, grandchildOnItsOwnProcessGroup);
		}
	}

	/* Wait to be terminated: */
	while (true) {
		sleep(DEFAULT_SLEEP_INTERVAL);
	}

	return EXIT_SUCCESS;
}

static bool checkGrandchildResultFile(int grandchildId) {
	char* fileName = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
	assert(fileName != NULL);
	sprintf(fileName, GRAND_CHILD_RESULT_FILE_NAME_FORMAT, grandchildId);

	char* expectedResultFileContent = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
	assert(expectedResultFileContent != NULL);
	sprintf(expectedResultFileContent, GRAND_CHILD_RESULT_FILE_CONTENT_FORMAT, grandchildId, strsignal(SIGHUP));

	char* actualFileContent = integrationReadFileContentAsString(fileName);

	bool result = strcmp(actualFileContent, expectedResultFileContent) == 0;

	free(fileName);
	free(expectedResultFileContent);
	free(actualFileContent);

	return result;
}

static void waitGrandchildrenFiles(const char* fileNameFormat) {
	char* fileName = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
	assert(fileName != NULL);

	while (true) {
		int readyCount = 0;
		for (int grandChildId = 1; grandChildId <= GRAND_CHILDREN_COUNT; grandChildId++) {
			sprintf(fileName, fileNameFormat, grandChildId);

			if (integrationTestFileExists(fileName)) {
				readyCount++;
			}
		}

		if (readyCount == GRAND_CHILDREN_COUNT) {
			break;
		} else {
			sleep(DEFAULT_SLEEP_INTERVAL);
		}
	}

	free(fileName);
}

static int doParent(const char* testCaseName, bool grandchildOnItsOwnProcessGroup) {
	pid_t sessionId = setsid();
	assert(sessionId == getpid());
	assert(getpgrp() == getpid());

	pid_t childProcessId = fork();
	assert(childProcessId != (pid_t) - 1);
	if (childProcessId == 0) {
		return doChild(grandchildOnItsOwnProcessGroup);
	}

	waitGrandchildrenFiles(GRAND_CHILD_READY_FILE_NAME_FORMAT);

	/* If child process finishes, each grandchild process will be in a orphaned process group. */
	kill(childProcessId, SIGQUIT);

	int status;
	pid_t processId = wait(&status);
	assert(processId == childProcessId);
	assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGQUIT);

	/* Check the result of each grandchild: */
	waitGrandchildrenFiles(GRAND_CHILD_RESULT_FILE_NAME_FORMAT);
	for (int grandChildId = 1; grandChildId <= GRAND_CHILDREN_COUNT; grandChildId++) {
		assert(checkGrandchildResultFile(grandChildId));
	}

	integrationTestRegisterSuccessfulCompletion(testCaseName);
	return EXIT_SUCCESS;
}

static char* createFolder(const char* testCaseName) {
	char* folderName = integrationTestCreateTemporaryFileName(testCaseName);
	int result = mkdir(folderName, 0);
	assert(result == 0);
	return folderName;
}

static void testCase1(const char* testCaseName) {
	char* folderName = createFolder(testCaseName);
	int result = chdir(folderName);
	assert(result == 0);

	result = doParent(testCaseName, true);
	assert(result == EXIT_SUCCESS);
}

static void testCase2(const char* testCaseName) {
	char* folderName = createFolder(testCaseName);
	int result = chdir(folderName);
	assert(result == 0);

	doParent(testCaseName, false);
	assert(result == EXIT_SUCCESS);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	testCase1(argv[0]);
	testCase2(argv[0]);

	return EXIT_SUCCESS;
}
