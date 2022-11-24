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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <myos.h>

#include "test/integration_test.h"

#include "util/double_linked_list.h"
#include "util/string_stream_writer.h"
#include "util/string_utils.h"

struct TestCase {
	struct DoubleLinkedListElement doubleLinkedListElement;
	pid_t processId;
	const char* name;
	bool terminatedNormally;
	bool terminated;
};

static struct DoubleLinkedList testCaseList;
static int terminatedTestCases = 0;

static pid_t runTestCase(const char* testCaseName, const char* testCaseExecutablePath) {
	pid_t forkResult = fork();
	assert(forkResult >= 0);

	if (forkResult == 0) {
		char buffer[PATH_MAX_LENGTH];
		struct StringStreamWriter stringStreamWriter;

		stringStreamWriterInitialize(&stringStreamWriter, buffer, PATH_MAX_LENGTH);
		streamWriterWriteString(&stringStreamWriter.streamWriter, INTEGRATION_TEST_OUTPUT_BASE_PATH, UINT_MAX);
		streamWriterWriteString(&stringStreamWriter.streamWriter, "/", UINT_MAX);
		streamWriterWriteString(&stringStreamWriter.streamWriter, testCaseName, UINT_MAX);
		streamWriterWriteString(&stringStreamWriter.streamWriter, ".txt", UINT_MAX);
		stringStreamWriterForceTerminationCharacter(&stringStreamWriter);

		int outputFileDescriptor = open(buffer, O_CREAT | O_WRONLY);
		assert(outputFileDescriptor != -1);
		int result = dup2(outputFileDescriptor, STDOUT_FILENO);
		assert(result != -1);
		result = dup2(outputFileDescriptor, STDERR_FILENO);
		assert(result != -1);
		if (outputFileDescriptor != STDOUT_FILENO && outputFileDescriptor != STDERR_FILENO) {
			close(outputFileDescriptor);
		}

		char* const argv[] = {(char*) testCaseName, NULL};

		execvpe(testCaseExecutablePath, argv, NULL);
	}

	return forkResult;
}

static bool handleChildTermination(pid_t processId, int exitStatus) {
	bool terminatedNormally = WIFEXITED(exitStatus) && WEXITSTATUS(exitStatus) == EXIT_SUCCESS;
	struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListFirst(&testCaseList);
	while (doubleLinkedListElement != NULL) {
		struct TestCase* testCase = (void*) doubleLinkedListElement;
		doubleLinkedListElement = doubleLinkedListElement->next;

		if (testCase->processId == processId) {
			assert(!testCase->terminated);
			testCase->terminated = true;
			testCase->terminatedNormally = terminatedNormally;
			terminatedTestCases++;
			break;
		}
	}
	return terminatedNormally;
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	assert(getsid(getpid()) == getpid());
	assert(getsid(0) == getpid());
	setsid();
	assert(getsid(getpid()) == getpid());
	assert(getsid(0) == getpid());

	int result = mkdir(INTEGRATION_TEST_OUTPUT_BASE_PATH, 0);
	assert(result == 0);

	/* Prepare STDOUT and STDERR. */
	int outputFileDescriptor = open(INTEGRATION_TEST_OUTPUT_BASE_PATH "test_suite.txt", O_CREAT | O_WRONLY);
	assert(outputFileDescriptor >= 0);
	result = dup2(outputFileDescriptor, STDOUT_FILENO);
	assert(result != -1);
	result = dup2(outputFileDescriptor, STDERR_FILENO);
	assert(result != -1);
	if (outputFileDescriptor != STDOUT_FILENO && outputFileDescriptor != STDERR_FILENO) {
		close(outputFileDescriptor);
	}

	/* Check if there are only two file descriptor indexes available. */
	for (int i = 0; i < OPEN_MAX; i++) {
		struct stat statInstance;
		int result = fstat(i, &statInstance);
		assert(result == -1 || (result == 0 && (i == STDERR_FILENO || i == STDOUT_FILENO)));
	}

	/* For each test case: */
	doubleLinkedListInitialize(&testCaseList);
	{
		DIR* devicesDirectory = opendir(INTEGRATION_TEST_EXECUTABLES_PATH);
		assert(devicesDirectory != NULL);
		struct dirent* directoryEntry;
		while ((directoryEntry = readdir(devicesDirectory)) != NULL) {
			char buffer[PATH_MAX_LENGTH];
			struct stat statInstance;
			struct StringStreamWriter stringStreamWriter;

			stringStreamWriterInitialize(&stringStreamWriter, buffer, PATH_MAX_LENGTH);
			streamWriterWriteString(&stringStreamWriter.streamWriter, INTEGRATION_TEST_EXECUTABLES_PATH, UINT_MAX);
			streamWriterWriteString(&stringStreamWriter.streamWriter, "/", UINT_MAX);
			streamWriterWriteString(&stringStreamWriter.streamWriter, directoryEntry->d_name, UINT_MAX);
			stringStreamWriterForceTerminationCharacter(&stringStreamWriter);

			result = stat(buffer, &statInstance);
			assert(result == 0);

			if (
				S_ISREG(statInstance.st_mode) && stringUtilsStartsWith(directoryEntry->d_name, "test_case_")
//					&& (
//							stringUtilsStartsWith(directoryEntry->d_name, "test_case_fork_2")
//						)
			) {
				struct TestCase* testCase = malloc(sizeof(struct TestCase));
				memset(testCase, 0, sizeof(struct TestCase));
				testCase->name = strdup(directoryEntry->d_name);
				testCase->processId = runTestCase(directoryEntry->d_name, buffer);
				doubleLinkedListInsertAfterLast(&testCaseList, &testCase->doubleLinkedListElement);
			}
		}
		closedir(devicesDirectory);
	}

	bool allChildrenTerminatedNormally = true;
	while (terminatedTestCases < doubleLinkedListSize(&testCaseList)) {
		int exitStatus;
		pid_t processId = wait(&exitStatus);
		if (processId > 0) {
			allChildrenTerminatedNormally = handleChildTermination(processId, exitStatus) && allChildrenTerminatedNormally;
		}
	}

	struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListFirst(&testCaseList);
	while (doubleLinkedListElement != NULL) {
		struct TestCase* testCase = (void*) doubleLinkedListElement;
		doubleLinkedListElement = doubleLinkedListElement->next;

		if (!testCase->terminatedNormally) {
			printf("Test case \"%s\" has not terminated normally.\n", testCase->name);
		}
	}

	if (allChildrenTerminatedNormally) {
		integrationTestRegisterSuccessfulCompletion("test_suite");
	}

	fflush(stderr);
	fflush(stdout);

//	myosLogKernelModuleDebugReport("process_manager");
//	myosLogKernelModuleDebugReport("process_group_manager");
//	myosLogKernelModuleDebugReport("session_manager");
//	myosLogKernelModuleDebugReport("virtual_file_system_manager");
//	sleep(3);

	myosReboot();

	return EXIT_SUCCESS;
}
