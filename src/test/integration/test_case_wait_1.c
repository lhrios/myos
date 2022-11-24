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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

static void doChild1(void) {
	char* argv[2];

	char buffer1[64];
	sprintf(buffer1, "%d", 5);
	argv[0] = buffer1;

	argv[1] = NULL;

	execvp(INTEGRATION_TEST_EXECUTABLES_PATH "executable_sleep", argv);
	assert(false);
	exit(EXIT_FAILURE);
}

static void doChild2(void) {
	sleep(10);
	exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	pid_t child1ProcessId = fork();
	if (child1ProcessId == 0) {
		doChild1();
	} else {
		assert(child1ProcessId > 0);
	}

	pid_t child2ProcessId = fork();
	if (child2ProcessId == 0) {
		doChild2();
	} else {
		assert(child2ProcessId > 0);
	}

	int status;
	pid_t result = waitpid(child1ProcessId, &status, 0);
	assert(child1ProcessId == result);
	assert(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
