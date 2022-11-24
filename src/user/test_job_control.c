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
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

FILE* logFile = NULL;

static void signalHandler(int signalId) {
	FILE* file = logFile != NULL ? logFile  : stdout;
	fprintf(file, "Handling signal \"%s\"\n", sys_siglist[signalId]);
}

static int blockSignal(int signalId) {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, signalId);
	if (sigprocmask(SIG_BLOCK, &set, NULL)) {
		perror(NULL);
		return -1;
	} else {
		return 0;
	}
}

static int unblockSignal(int signalId) {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, signalId);
	if (sigprocmask(SIG_UNBLOCK, &set, NULL)) {
		perror(NULL);
		return -1;
	} else {
		return 0;
	}
}

static int handleSignal(int signalId, void (*handler)(int)) {
	struct sigaction act;
	memset(&act, sizeof(sigaction), 0);
	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	if (sigaction(signalId, &act, NULL)) {
		perror(NULL);
		return -1;
	} else {
		return 0;
	}
}

enum Operation {
	DO_NOTHING,
	READ,
	WRITE,
	SET_FOREGROUND_PROCESS_GROUP,
	SET_TTY_CONFIGURATION
};


int main(int argc, char** argv) {
   static struct option options[] = {
       {"blockSigttin", no_argument, 0, 0},
       {"ignoreSigttin", no_argument, 0, 0},
       {"blockSigttou", no_argument, 0, 0},
       {"ignoreSigttou", no_argument, 0, 0},
       {"read", no_argument, 0, 0},
       {"write", no_argument, 0, 0},
       {"sleep", required_argument, 0, 0},
       {"handleSigttin", no_argument, 0, 0},
       {"handleSigttou", no_argument, 0, 0},
       {"fork", no_argument, 0, 0},
       {"logFile", required_argument, 0, 0},
       {"quiet", no_argument, 0, 0},
       {"setForegroundProcessGroup", no_argument, 0, 0},
       {"setTTYConfiguration", no_argument, 0, 0},
       {0, 0, 0, 0}
   };

   enum Operation operation = DO_NOTHING;
   bool quiet = false;
   bool mustFork = false;
   int secondsToSleep = 5;

   int longOptionIndex;
   int result;
   while (true) {
   	result = getopt_long(argc, argv, "", options, &longOptionIndex);
   	if (result != 0) {
   		break;
   	} else {
			switch (longOptionIndex) {
				case 0:
					if (!quiet) {
						printf("blockSigttin\n");
					}
					if (blockSignal(SIGTTIN)) {
						return EXIT_FAILURE;
					}
					break;

				case 1:
					if (!quiet) {
						printf("ignoreSigttin\n");
					}
					if (handleSignal(SIGTTIN, SIG_IGN)) {
						return EXIT_FAILURE;
					}
					break;

				case 2:
					if (!quiet) {
						printf("blockSigttou\n");
					}
					if (blockSignal(SIGTTOU)) {
						return EXIT_FAILURE;
					}
					break;

				case 3:
					if (!quiet) {
						printf("ignoreSigttou\n");
					}
					if (handleSignal(SIGTTOU, SIG_IGN)) {
						return EXIT_FAILURE;
					}
					break;

				case 4:
					if (!quiet) {
						printf("read\n");
					}
					operation = READ;
					break;

				case 5:
					if (!quiet) {
						printf("write\n");
					}
					operation = WRITE;
					break;

				case 6:
					secondsToSleep = atoi(optarg);
					if (!quiet) {
						printf("sleep %d seconds\n", secondsToSleep);
					}
					break;

				case 7:
					if (!quiet) {
						printf("handleSigttin\n");
					}
					if (handleSignal(SIGTTIN, &signalHandler)) {
						return EXIT_FAILURE;
					}
					break;

				case 8:
					if (!quiet) {
						printf("handleSigttou\n");
					}
					if (handleSignal(SIGTTOU, &signalHandler)) {
						return EXIT_FAILURE;
					}
					break;

				case 9:
					if (!quiet) {
						printf("fork\n");
					}
					mustFork = true;
					break;

				case 10:
					if (!quiet) {
						printf("logFile\n");
					}
					logFile = fopen(optarg, "w");
					if (logFile == NULL) {
						printf("couldn't open file \"%s\" to write on it", optarg);
						return EXIT_FAILURE;
					}
					setvbuf(logFile, NULL, _IONBF, 0);
					break;

				case 11:
					quiet = true;
					break;


				case 12:
					if (!quiet) {
						printf("setForegroundProcessGroup\n");
					}
					operation = SET_FOREGROUND_PROCESS_GROUP;
					break;


				case 13:
					if (!quiet) {
						printf("setTTYConfiguration\n");
					}
					operation = SET_TTY_CONFIGURATION;
					break;
			}
   	}
   }

   handleSignal(SIGHUP, &signalHandler);

   if (operation == DO_NOTHING) {
   	printf("no operation has been selected\n");
   	return EXIT_FAILURE;
   }


   if (mustFork) {
   	pid_t childProcessId = fork();
   	if (childProcessId < 0) {
   		perror(NULL);
   		return EXIT_FAILURE;
   	} else if (childProcessId != 0) {
   		return EXIT_SUCCESS;
   	}
   }

   sleep(secondsToSleep);

	FILE* file = logFile != NULL ? logFile  : stdout;

   switch (operation) {
   	case READ: {
   		int result;
			char c;
			while((result = read(STDIN_FILENO, &c, 1)) == 1) {
				if (isascii(c)) {
					fprintf(file, "read %d %c\n", c, c);
				} else {
					fprintf(file, "read %d\n", c);
				}
			}

			if (result == 0) {
				fprintf(file, "found EOF while reading from stdin\n");
			} else {
				assert(result < 0);
				fprintf(file, "found error while reading from stdin: %s\n", strerror(errno));
			}

   	} break;

   	case WRITE: {
			const char* message = "writing...\n";
			int result = write(STDOUT_FILENO, message, strlen(message));
			if (result <= 0 && logFile != NULL) {
				fprintf(logFile, "found error while writing to stdout: %s\n", strerror(errno));
			}
   	} break;

   	case SET_FOREGROUND_PROCESS_GROUP: {
   		int result = tcsetpgrp(STDIN_FILENO, getpgrp());
			if (result < 0 && logFile != NULL) {
				fprintf(logFile, "found error while setting foreground process group: %s\n", strerror(errno));
			}
   	} break;

   	case SET_TTY_CONFIGURATION: {
   		struct termios termiosInstance, termiosInitialInstance;
   		int result = tcgetattr(STDOUT_FILENO, &termiosInstance);
   		if (result < 0 && logFile != NULL) {
   			fprintf(logFile, "found error while executing tcgetattr: %s\n", strerror(errno));
   		}
   		memcpy(&termiosInitialInstance, &termiosInstance, sizeof(struct termios));
   		termiosInstance.c_lflag &= (~ECHO & ~ICANON);
   		result = tcsetattr(STDIN_FILENO, TCSANOW, &termiosInstance);
   		if (result < 0 && logFile != NULL) {
   			fprintf(logFile, "found error while executing tcsetattr: %s\n", strerror(errno));
   		} else {
   			if (!quiet) {
      			fprintf(file, "successfully changed tty configuration\n");
   			}
   			tcsetattr(STDIN_FILENO, TCSANOW, &termiosInitialInstance);
   		}
   	} break;

   	default:
   		assert(false);
   		return EXIT_FAILURE;
   		break;
   }

   unblockSignal(SIGTTIN);
   unblockSignal(SIGTTOU);

   fprintf(file, "exiting...\n");
   if (logFile != NULL) {
   	fclose(logFile);
   }

	return EXIT_SUCCESS;
}
