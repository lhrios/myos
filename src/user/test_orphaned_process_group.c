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

/*
 * The following code is an adaptation of the code proposed on figure 9.12 (creating an orphaned process group)
 * present on chapter 9 of the book Advanced Programming in the Unix Environment, third edition.
 */

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

enum OperationToRaiseSignal {
	READ,
	TCSETATTR,
	TCSETPGRP
};
const char* NAMES_OF_OPERATION_TO_RAISE_SIGNAL[] = {
	"READ",
	"TCSETATTR",
	"TCSETPGRP"
};

enum OperationToRaiseSignal operationToRaiseSignal = READ;

static void handleSignal(int signo) {
	printf("SIGHUP received, pid = %ld\n", (long)getpid());
}

static void printProcessInformation(char *name) {
	printf(
		"%s: pid=%ld, ppid=%ld, pgrp=%ld, sid=%ld, tpgrp=%ld\n",
		name,
		(long) getpid(),
		(long) getppid(),
		(long) getpgrp(),
		(long) getsid(0),
		(long) tcgetpgrp(STDIN_FILENO)
	);
	fflush(stdout);
}

static void parseCommandLine(int argc, char** argv) {
	int result;
	while ((result = getopt(argc, argv, "raf")) != -1) {
		if (result == 'r') {
			operationToRaiseSignal = READ;
		} else if (result == 'a') {
			operationToRaiseSignal = TCSETATTR;
		} else if (result == 'f') {
			operationToRaiseSignal = TCSETPGRP;
		}
	}
}

int main(int argc, char** argv) {
	char c;
	pid_t	pid;

	printProcessInformation("parent");
	if ((pid = fork()) < 0) {
		perror("fork error");

	} else if (pid > 0) {
		/* parent */

		sleep(5);
		/* sleep to let child stop itself */

	} else {
		parseCommandLine(argc, argv);

		printProcessInformation("child");
		/* child */

		signal(SIGHUP, handleSignal);
		/* establish signal handler */

		kill(getpid(), SIGTSTP);
		/* stop ourself */

		printProcessInformation("child");
		/* prints only if we're continued */

		bool error;
		switch (operationToRaiseSignal) {
			default:
			case READ:
				error = read(STDIN_FILENO, &c, 1) != 1;
				break;

			case TCSETATTR: {
				struct termios termiosInstance;
				error = tcgetattr(STDIN_FILENO, &termiosInstance) != 0;
				assert(!error);
				error = tcsetattr(STDIN_FILENO, TCSANOW, &termiosInstance) != 0;
			} break;

			case TCSETPGRP:
				error = tcsetpgrp(STDIN_FILENO, getpgrp());
			break;
		}
		if (error) {
			printf("error (%s) while executing operation \"%s\" on controlling TTY\n",
				sys_errlist[errno],
				NAMES_OF_OPERATION_TO_RAISE_SIGNAL[operationToRaiseSignal]);
		}

		printf("exiting...");
	}

	return EXIT_SUCCESS;
}
