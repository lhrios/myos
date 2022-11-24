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

#include <myos.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <sys/wait.h>

#include "util/string_utils.h"

#include "user/util/dynamic_array.h"

static const int FIRST_USER_AVAILABLE_TTY_ID = 3;
static pid_t* processIdByTtyId = NULL;
static int userAvailableTtyCount = 0;

static void handleSignal(int signalId) {
	printf("Caught signal (%s).\n", strsignal(signalId));
}

static void doChild(int ttyId) {
	int result;
	char buffer[128];

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	result = setsid();
	myosSystemAssert(result == getpid());
	myosSystemAssert(getpgrp() == getpid());
	myosSystemAssert(getpgid(getpid()) == getpid());
	myosSystemAssert(getsid(getpid()) == getpid());

	sprintf(buffer, "/dev/tty%.2d", ttyId);

	result = open(buffer, O_RDONLY);
	assert(result == STDIN_FILENO);
	result = open(buffer, O_WRONLY);
	assert(result == STDOUT_FILENO);
	result = open(buffer, O_WRONLY);
	assert(result == STDERR_FILENO);

	result = tcgetsid(STDIN_FILENO);
	myosSystemAssert(result == getpid());
	result = tcgetsid(STDOUT_FILENO);
	myosSystemAssert(result == getpid());
	result = tcgetsid(STDERR_FILENO);
	myosSystemAssert(result == getpid());

	result = tcsetpgrp(STDIN_FILENO, getpid());
	myosSystemAssert(result == 0);

	char* envp[5];
	envp[0] = "USER=root";
	envp[1] = "PATH=/bin/:/debug/:/usr/bin/:/sbin/";
	envp[2] = "HOME=/home/";
	envp[3] = "TERM=myos";
	envp[4] = NULL;

	char* argv[2];
	argv[0] = "-bash"; /* "A login shell is one whose first character of argument zero is a -, or one started with the --login option." from Bash manual. */
	argv[1] = NULL;

	if (execvpe("/bin/sh", argv, envp) != 0) {
		if (errno == ENOENT) {
			execvpe("/debug/system_call_tester", argv, envp);
		}
		perror(NULL);
	}

	sleep(60);
	exit(EXIT_FAILURE);
}

static void handleChildSignal(int signalId) {
	int status;
	while (true) {
		pid_t childProcessId = waitpid(-1, &status, WCONTINUED | WNOHANG | WUNTRACED);
		if (childProcessId > 0) {
			bool newChild = false;
			printf("It returned information (%X) about a child (id=%d):\n", status, childProcessId);
			if (WIFEXITED(status)) {
				newChild = true;
				printf("  it terminated normally returning %d\n", WEXITSTATUS(status));
			}

			if (WIFSIGNALED(status)) {
				newChild = true;
				printf("  it terminated abnormally with a signal (%s) that has not been caught\n", strsignal(WTERMSIG(status)));
			}

			if (WIFSTOPPED(status)) {
				printf("  it is currently stopped due to signal %s\n", strsignal(WSTOPSIG(status)));
			}

			if (WIFCONTINUED(status)) {
				printf("  it resumed its normal execution\n", WSTOPSIG(status));
			}

			if (newChild) {
				for (int ttyId = FIRST_USER_AVAILABLE_TTY_ID; ttyId < FIRST_USER_AVAILABLE_TTY_ID + userAvailableTtyCount; ttyId++) {
					if (processIdByTtyId[ttyId - FIRST_USER_AVAILABLE_TTY_ID] == childProcessId) {
						pid_t childProcessId = fork();
						if (childProcessId != (pid_t)-1) {
							if (childProcessId != 0) {
								processIdByTtyId[ttyId - FIRST_USER_AVAILABLE_TTY_ID] = childProcessId;
								printf("I am %u's parent. I am %u!\n", childProcessId, getpid());
							} else {
								doChild(ttyId);
							}
						} else {
							processIdByTtyId[ttyId - FIRST_USER_AVAILABLE_TTY_ID] = (pid_t) -1;
						}
					}
				}
			}

		} else if (childProcessId == -1) {
			perror(NULL);
			break;

		} else {
			break;
		}
	}
}

int main(int argc, char** argv) {
	int result;

	struct sigaction act;
	act.sa_flags = 0;
	act.sa_handler = &handleSignal;
	act.sa_sigaction = NULL;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGUSR1, &act, NULL)) {
		perror(NULL);
	}

	act.sa_flags = 0;
	act.sa_handler = &handleSignal;
	act.sa_sigaction = NULL;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGUSR2, &act, NULL)) {
		perror(NULL);
	}

	act.sa_flags = 0;
	act.sa_handler = &handleChildSignal;
	act.sa_sigaction = NULL;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGCHLD, &act, NULL)) {
		perror(NULL);
	}

	act.sa_flags = 0;
	act.sa_handler = SIG_IGN;
	act.sa_sigaction = NULL;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGTTOU, &act, NULL)) {
		perror(NULL);
	}

	result = setsid();
	myosSystemAssert(result == 1);

	result = open("/dev/tty02", O_RDONLY | O_NOCTTY);
	myosSystemAssert(result == 0);

	result = open("/dev/tty02", O_WRONLY | O_NOCTTY);
	myosSystemAssert(result == 1);

	result = open("/dev/tty02", O_WRONLY | O_NOCTTY);
	myosSystemAssert(result == 2);

	printf("argc=%d\n", argc);
	for (int i = 0; i < argc; i++) {
		printf("argv[%d]=%s\n", i, argv[i]);
	}

	if (environ != NULL && environ[0] != NULL) {
		int i = 0;
		while (environ[i] != NULL) {
			printf("environ[%d]=%s\n", i, environ[i]);
			i++;
		}
	} else {
		printf("environ is empty\n");
	}

	printf("I am \"init\" and my process id is %d\n", getpid());

	DIR* devicesDirectory = opendir("/dev/");
	if (devicesDirectory == NULL) {
		perror(NULL);
		myosSystemAssert(false);

	} else {
		struct dirent* directoryEntry;
		while ((directoryEntry = readdir(devicesDirectory)) != NULL) {
			if (stringUtilsStartsWith(directoryEntry->d_name, "tty")) {
				int ttyId = atoi(directoryEntry->d_name + 3);
				if (ttyId >= FIRST_USER_AVAILABLE_TTY_ID) {
					userAvailableTtyCount++;
				}
			}
		}
		closedir(devicesDirectory);

		processIdByTtyId = malloc(sizeof(pid_t) * userAvailableTtyCount);
		for (int ttyId = FIRST_USER_AVAILABLE_TTY_ID; ttyId < FIRST_USER_AVAILABLE_TTY_ID + userAvailableTtyCount; ttyId++) {
			pid_t childProcessId = fork();
			if (childProcessId != (pid_t)-1) {
				if (childProcessId != 0) {
					processIdByTtyId[ttyId - FIRST_USER_AVAILABLE_TTY_ID] = childProcessId;
					printf("I am %d's parent. I am %d\n", childProcessId, getpid());
				} else {
					doChild(ttyId);
				}
			} else {
				processIdByTtyId[ttyId - FIRST_USER_AVAILABLE_TTY_ID] = (pid_t) -1;
			}
		}
	}

	/* The "init" process can not return. */
	while (true) {
		// TODO: Use sigsuspend
		sleep(15);
	}

	return EXIT_SUCCESS;
}
