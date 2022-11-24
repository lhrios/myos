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
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "kernel/limits.h"

#include "util/command_line_utils.h"
#include "util/debug_utils.h"
#include "util/double_linked_list.h"
#include "util/math_utils.h"
#include "util/string_utils.h"
#include "util/string_stream_writer.h"

#include "user/util/tty_utils.h"

static const char* PROMPT = "$> ";

#define COMMAND_LINE_MAX_LENGTH 1023
static char commandLine[COMMAND_LINE_MAX_LENGTH + 1];
static int commandLineLength = 0;

#define MAX_HISTORICAL_COMMAND_LINES 10
struct DoubleLinkedList historicalCommandLinesList;
struct CommandLineHistory {
	struct DoubleLinkedListElement doubleLinkedListElement;
	char commandLine[COMMAND_LINE_MAX_LENGTH + 1];
};
struct DoubleLinkedListElement* lastHistoricalCommandLineRestored = NULL;

volatile int timesToRaiseSIGUSR1 = 0;
volatile bool mustSleep;

static void handleSignal(int signalId) {
	switch (signalId) {
		case SIGUSR1: {
			fprintf(stderr, "Caught SIGUSR1.\n");

			if (timesToRaiseSIGUSR1 > 0) {
				fprintf(stderr, "%d\n", timesToRaiseSIGUSR1);
				if (mustSleep) {
					sleep(10);
				}

				timesToRaiseSIGUSR1--;
				fprintf(stderr, "Will raise %s.\n", strsignal(SIGUSR1));
				raise(SIGUSR1);
				fprintf(stderr, "After raise %s.\n", strsignal(SIGUSR1));

			} else {
				sigset_t signalSet;
				sigemptyset(&signalSet);
				sigaddset(&signalSet, SIGUSR2);
				if (sigprocmask(SIG_UNBLOCK, &signalSet, NULL)) {
					perror(NULL);
				}
			}
		} break;

		case SIGUSR2: {
			fprintf(stderr, "Caught SIGUSR2.\n");
		} break;

		case SIGTSTP: {
			fprintf(stderr, "Caught SIGTSTP. I will stop myself.\n");
			raise(SIGSTOP);
		} break;

		case SIGCONT: {
			fprintf(stderr, "Caught SIGCONT. I will resume normal execution.\n");
		} break;

		case SIGSEGV: {
			fprintf(stderr, "Caught SIGSEGV.\n");
			sleep(5);
			//abort();
		} break;

		case SIGHUP: {
			fprintf(stderr, "Caught SIGHUP.\n");
			exit(EXIT_SUCCESS);
		} break;

		case SIGSYS: {
			fprintf(stderr, "Caught SIGSYS.\n");
			sleep(5);
			//abort();
		} break;

		default:
			fprintf(stderr, "Caught unknown signal (%d).\n", signalId);
			break;
	}
}

static void sendSignalToProcess(pid_t processId, int signalId) {
	printf("Sending signal %s to process %d\n", strsignal(signalId), processId);

	if (kill(processId, signalId)) {
		perror(NULL);
	}
}

struct DoubleLinkedList commands;

struct Command {
	struct DoubleLinkedListElement doubleLinkedListElement;
	const char* id;
	void (*action)(int, const char**);
};

static void actionReboot(int argc, const char** argv) {
	myosReboot();
}

static void actionReadFromAddress(int argc, const char** argv) {
	if (argc == 1) {
		char* string;
		errno = 0;
		uint32_t address = strtoul(argv[0], &string, 0);
		if (strcmp(string, "") == 0) {
			uint32_t value;
			__asm__ __volatile__(
				"mov (%%eax), %%eax;"
				"mov %%eax, %1;"
				:
				: "a"(address), "m"(value)
				: "memory");
			printf("value at address %X is %X\n", address, value);
		}
	}
}

static void actionDescribeOpenFileDescription(int argc, const char** argv) {
	int fileDescriptorIndex = 0;
	if (argc == 1 && 1 == sscanf(argv[0], "%d", &fileDescriptorIndex)) {
		int result = fcntl(fileDescriptorIndex, F_GETFL);
		if (result == -1) {
			perror(NULL);
		} else {
			printf("\tF_GETFL=%X\n", result);
		}
	}
}

static void actionCommonStat(int argc, const char** argv, bool followLastSymbolicLink) {
	const char* path = NULL;
	if (argc == 1) {
		path = argv[0];
	} else {
		path = ".";
	}

	printf("%s\n", argv[0]);

	struct stat statInstance;

	int result;
	if (followLastSymbolicLink) {
		result = stat(path, &statInstance);
	} else {
		result = lstat(path, &statInstance);
	}

	if (result) {
		perror(NULL);

	} else {
		printf("  st_dev=%X\n", statInstance.st_dev);
		printf("  st_ino=%d\n", statInstance.st_ino);
		printf("  st_size=%d\n", statInstance.st_size);
		printf("  st_mode=%X\n", statInstance.st_mode);

		printf("  st_atime=%s\n", ctime(&statInstance.st_atime));
		printf("  st_ctime=%s\n", ctime(&statInstance.st_ctime));
		printf("  st_mtime=%s\n", ctime(&statInstance.st_mtime));

		if (S_ISCHR(statInstance.st_mode) || S_ISBLK(statInstance.st_mode)) {
			printf("  st_rdev=%X\n", statInstance.st_rdev);
		}

		printf("  file type=");
		if (S_ISBLK(statInstance.st_mode)) {
			printf("block device");

		} else if (S_ISCHR(statInstance.st_mode)) {
			printf("character device");

		} else if (S_ISDIR(statInstance.st_mode)) {
			printf("directory");

		} else if (S_ISFIFO(statInstance.st_mode)) {
			printf("FIFO/pipe");

		} else if (S_ISLNK(statInstance.st_mode)) {
			printf("symlink");

		} else if (S_ISREG(statInstance.st_mode)) {
			printf("regular file");

		} else if (S_ISSOCK(statInstance.st_mode)) {
			printf("socket");

		} else {
			printf("unknown\n");
		}

		printf("\n");
	}
}

static void actionTestRandom(int argc, const char** argv) {
	int* count = malloc(sizeof(int) * 6);
	memset(count, 0, sizeof(int) * 6);

	srand((uint32_t) time(NULL));
	int totalCount = 100000;
	for (int i = 0; i < totalCount; i++) {
		count[rand() % 6]++;
	}

	for (int i = 0; i < 6; i++) {
		printf("%d: %.2f%%\n", i + 1, ((double) count[i]) / totalCount * 100.);
	}

	free(count);
}

static void actionStat(int argc, const char** argv) {
	actionCommonStat(argc, argv, true);
}

static void actionLStat(int argc, const char** argv) {
	actionCommonStat(argc, argv, false);
}

static void actionSlowList(int argc, const char** argv) {
	const char* path = NULL;
	if (argc == 1) {
		path = argv[0];
	} else {
		path = ".";
	}

	DIR* devicesDirectory = opendir(path);
	if (devicesDirectory == NULL) {
		perror(NULL);

	} else {
		struct dirent* directoryEntry;
		while ((directoryEntry = readdir(devicesDirectory)) != NULL) {
			sleep(3);
			printf("%s\n\t", directoryEntry->d_name);
			const char* string = directoryEntry->d_name;
			while (*string != '\0') {
				printf("%2d ", *string);
				string++;
			}
			printf("\n");
		}
		closedir(devicesDirectory);
	}
}

static void actionAbort(int argc, const char** argv) {
	abort();
}

static void actionHelp(int argc, const char** argv) {
	struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListFirst(&commands);
	while (doubleLinkedListElement != NULL) {
		struct Command* command = (void*) doubleLinkedListElement;
		doubleLinkedListElement = doubleLinkedListElement->next;

		printf("%s\n", command->id);
	}
}

static void actionCreateName(int argc, const char** argv) {
	if (argc == 2) {
		const char* targetPath = argv[0];
		const char* namePathToCreate = argv[1];
		if (link(targetPath, namePathToCreate) != 0) {
			perror(NULL);
		}
	}
}

static void actionReleaseName(int argc, const char** argv) {
	if (argc == 1) {
		if (unlink(argv[0]) != 0) {
			perror(NULL);
		}
	}
}

static void actionReleaseDirectory(int argc, const char** argv) {
	if (argc >= 1) {
		if (rmdir(argv[0]) != 0) {
			perror(NULL);
		}
	}
}

static void actionRaiseSIGSEGV(int argc, const char** argv) {
	raise(SIGSEGV);
}

static void actionRaiseSIGWINCH(int argc, const char** argv) {
	raise(SIGWINCH);
}

static void actionSystemAssertFalse(int argc, const char** argv) {
	myosSystemAssert(false);
}

static void actionSendSignal(int argc, const char** argv) {
	pid_t processId;
	if (argc == 2) {
		const char* signalName = argv[0];
		if (1 == sscanf(argv[1], "%d", &processId)) {
			int signalId = -1;
			if (strcmp(signalName, "0") == 0) {
				signalId = 0;

			} else {
				for (int i = 1; i <= NUMBER_OF_SIGNALS; i++) {
					if (strcmp(signalName, sys_siglist[i]) == 0) {
						signalId = i;
						break;
					}
				}
			}

			if (signalId != -1) {
				sendSignalToProcess(processId, signalId);
			}
		}
	}
}

static void actionQuit(int argc, const char** argv) {
	exit(EXIT_SUCCESS);
}

static void actionSeekStdout(int argc, const char** argv) {
	if (lseek(STDOUT_FILENO, 15, SEEK_CUR)) {
		perror(NULL);
	}
}

static void actionStressGlob(int argc, const char** argv) {
	if (argc >= 2) {
		int count = atoi(argv[0]);
		while (count-- > 0) {
			glob_t globInstance;
			int result = glob(argv[1], 0, NULL, &globInstance);
			if (result == 0) {
				globfree(&globInstance);
			}
		}
	}
}

static void actionMalloc(int argc, const char** argv) {
	if (argc >= 2) {
		int size = atoi(argv[0]);
		if (!malloc(size)) {
			printf("Error\n");
		}
	}
}

static void actionStressCPU(int argc, const char** argv) {
	time_t timeBefore = time(NULL);
	sleep(15);
	for (uint32_t i = 0; i < 2; i++) {
		for (uint32_t j = 0; j < 0xFFFFFFFF; j++) {
			__asm__ __volatile__(
				"nop;nop;nop;nop;nop;nop;nop;nop;"
				:
				:
				: "memory");
			if (j % 0xFFFFFF == 0) {
				printf("\rdoing %03d...", j / 0xFFFFFF);
				fflush(stdout);
			}
		}
	}
	time_t timeAfter = time(NULL);
	printf("\nDone! It took %d seconds.\n", mathUtilsMax(0, timeAfter - timeBefore));
}

static void actionForceSIGSEGV(int argc, const char** argv) {
	int* pointer = NULL;
	(*pointer)++;
}

static void actionForceKernelSIGSEGV(int argc, const char** argv) {
	myosForceKernelSIGSEGV();
}

static void actionForceKernelSIGILL(int argc, const char** argv) {
	myosForceKernelSIGILL();
}

static void actionForceDivisionByZero(int argc, const char** argv) {
	pid_t processId = getpid();
	printf("%d", 1 / (processId - getpid()));
}

static void actionRecursiveSignalHandler(int argc, const char** argv) {
	timesToRaiseSIGUSR1 = 3;
	mustSleep = true;
	raise(SIGUSR1);
}

static void actionAssertFalse(int argc, const char** argv) {
	assert(true == false);
}

static void actionInfinitySignalHandler(int argc, const char** argv) {
	timesToRaiseSIGUSR1 = INT_MAX;
	mustSleep = false;
	raise(SIGUSR1);
}

static void actionSleep(int argc, const char** argv) {
	uint32_t seconds;
	if (argc == 0 || 1 != sscanf(argv[0], "%u", &seconds)) {
		seconds = 15;
	}

	printf("I will be sleeping during next %u seconds.\n", seconds);
	uint32_t secondsLeftToSleep = sleep(seconds);
	printf("Seconds left to sleep: %u seconds.\n", secondsLeftToSleep);
}

static void actionGetCWD(int argc, const char** argv) {
	char* currentWorkingDirectory = getcwd(NULL, 0);
	if (currentWorkingDirectory == NULL) {
		perror(NULL);
	} else {
		printf("currentWorkingDirectory='%s'\n", currentWorkingDirectory);
		free(currentWorkingDirectory);
	}
}

static void actionSetCWD(int argc, const char** argv) {
	if (argc == 1) {
		if (chdir(argv[0])) {
			perror(NULL);
		}
	}
}

static void actionTestPoll(int argc, const char** argv) {
	if (argc >= 1) {
		int timeout = atoi(argv[0]);
		int i = 0;
		while (i++ <= 2) {
			struct pollfd ioEventMonitoringContexts[1];
			nfds_t ioEventMonitoringContextCount = 1;

			ioEventMonitoringContexts[0].events = POLLIN;
			ioEventMonitoringContexts[0].fd = 0;

			printf("Before poll\n");
			int result = poll(ioEventMonitoringContexts, ioEventMonitoringContextCount, timeout);
			printf("poll\n  result=%d\n", result);
			if (result == -1) {
				perror(NULL);
			} else {
				printf("  revents=%d\n", ioEventMonitoringContexts[0].revents);
			}

			char buffer[128];
			result = read(0, buffer, 128);
			if (result == -1) {
				perror(NULL);
			} else {
				printf("result=%d\n", result);
			}
		}
	}
}

static void actionTestPoll2(int argc, const char** argv) {
	const nfds_t ioEventMonitoringContextCount = 4;
	struct pollfd ioEventMonitoringContexts[ioEventMonitoringContextCount];

	int tty1 = open("/dev/tty01", O_RDONLY);
	if (tty1 < 0) {
		perror(NULL);

	} else {
		ioEventMonitoringContexts[0].events = POLLIN;
		ioEventMonitoringContexts[0].fd = tty1;

		int tty2 = open("/dev/tty02", O_RDONLY);
		if (tty2 < 0) {
			perror(NULL);

		} else {
			ioEventMonitoringContexts[1].events = POLLIN;
			ioEventMonitoringContexts[1].fd = tty2;

			int tty3 = open("/dev/tty03", O_RDONLY);
			if (tty3 < 0) {
				perror(NULL);

			} else {
				ioEventMonitoringContexts[2].events = POLLIN;
				ioEventMonitoringContexts[2].fd = tty3;

				int tty4 = open("/dev/tty04", O_RDONLY);
				if (tty4 < 0) {
					perror(NULL);

				} else {
					ioEventMonitoringContexts[3].events = POLLIN;
					ioEventMonitoringContexts[3].fd = tty4;
					
					printf("Before poll\n");
					int result = poll(ioEventMonitoringContexts, ioEventMonitoringContextCount, 0);
					printf("poll\n  result=%d\n", result);
					if (result == -1) {
						perror(NULL);
					} else {
						printf("  revents[0]=%d %d\n", ioEventMonitoringContexts[0].revents, tty1);
						printf("  revents[1]=%d %d\n", ioEventMonitoringContexts[1].revents, tty2);
						printf("  revents[2]=%d %d\n", ioEventMonitoringContexts[2].revents, tty3);
						printf("  revents[3]=%d %d\n", ioEventMonitoringContexts[3].revents, tty4);
					}

					close(tty1);
					close(tty2);
					close(tty3);
					close(tty4);
				}
			}
		}
	}
}

static void actionTestPoll3(int argc, const char** argv) {
	const nfds_t ioEventMonitoringContextCount = 0;
	struct pollfd ioEventMonitoringContexts[ioEventMonitoringContextCount];

	printf("Before poll\n");
	int result = poll(ioEventMonitoringContexts, ioEventMonitoringContextCount, 15 * 1000);
	printf("poll\n  result=%d\n", result);
	if (result == -1) {
		perror(NULL);
	} else {
		printf("Success\n");
	}
}

static void actionRename(int argc, const char** argv) {
	if (argc >= 2) {
		int result = rename(argv[0], argv[1]);
		if (result != 0) {
			perror(NULL);
		}
	}
}

static void actionSystem(int argc, const char** argv) {
	char* buffer = malloc(sizeof(char) * 4096);
	buffer[0] = '\0';
	for (int i = 0; i < argc; i++) {
		if (i > 0) {
			strcat(buffer, " ");
		}
		strcat(buffer, argv[i]);
	}

	int result = system(buffer);
	printf("system has been executed and returned %d\n", result);

	if (WIFEXITED(result)) {
		printf("  it terminated normally returning %d\n", WEXITSTATUS(result));
	}

	if (WIFSIGNALED(result)) {
		printf("  it terminated abnormally with a signal (%s) that has not been caught\n", strsignal(WTERMSIG(result)));
	}

	if (WIFSTOPPED(result)) {
		printf("  it is currently stopped due to signal %s\n", strsignal(WSTOPSIG(result)));
	}

	if (WIFCONTINUED(result)) {
		printf("  it resumed its normal execution\n", WSTOPSIG(result));
	}
}

static void actionTestPoll4(int argc, const char** argv) {
	const nfds_t ioEventMonitoringContextCount = rand() % 3;
	struct pollfd ioEventMonitoringContexts[ioEventMonitoringContextCount];

	for (int i = 0; i < ioEventMonitoringContextCount; i++) {
		ioEventMonitoringContexts[i].events = POLLIN;
		ioEventMonitoringContexts[i].fd = (rand() % 64) * (rand() % 2 == 0 ? 1 : -1);
		ioEventMonitoringContexts[i].revents = rand();
	}

	printf("Before poll\n");
	int result = poll(ioEventMonitoringContexts, ioEventMonitoringContextCount, 15 * 1000);
	printf("poll\n  result=%d\n", result);
	if (result == -1) {
		perror(NULL);
	} else {
		printf("Success\n");
	}
}

static void writeHexadecimalTable(void* buffer, size_t count) {
	const int stringLength = 4096 * 2;
	char string[stringLength];
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, string, stringLength);

	debugUtilsBuildHexadecimalTable(&stringStreamWriter.streamWriter, buffer, count);
	write(STDOUT_FILENO, string, streamWriterGetWrittenCharacterCount(&stringStreamWriter.streamWriter));
}

static void readFileContent(int argc, const char** argv, bool isTextFile) {
	int offset = 0;
	if (argc >= 3) {
		sscanf(argv[2], "%i", &offset);
	}

	int charactersToRead = INT_MAX;
	if (argc >= 2) {
		sscanf(argv[1], "%i", &charactersToRead);
	}

	if (argc >= 1) {
		const char* path = argv[0];
		int bufferSize = PATH_MAX_LENGTH;
		uint8_t buffer[bufferSize];

		int fileDescriptorIndex = open(path, O_RDONLY);
		if (fileDescriptorIndex != -1) {
			if (lseek(fileDescriptorIndex, offset, offset >= 0 ? SEEK_SET : SEEK_END) == -1) {
				perror(NULL);

			} else {
				size_t count;
				do {
					count = read(fileDescriptorIndex, buffer, mathUtilsMin(charactersToRead, bufferSize));
					if (count == -1) {
						perror(NULL);
						break;
					} else if (count > 0) {
						if (isTextFile) {
							if (write(STDOUT_FILENO, buffer, count) == -1) {
								perror(NULL);
								break;
							}
						} else {
							writeHexadecimalTable(buffer, count);
						}
					}
					charactersToRead -= count;
				} while (count != 0 && charactersToRead > 0);
			}

			close(fileDescriptorIndex);

		} else {
			perror(NULL);
		}
	}
}

static void actionReadTextFileContent(int argc, const char** argv) {
	readFileContent(argc, argv, true);
}

static void actionReadBinaryFileContent(int argc, const char** argv) {
	readFileContent(argc, argv, false);
}

static void actionReadMemoryContent(int argc, const char** argv) {
	if (argc >= 2) {
		uint32_t address = strtoul(argv[0], NULL, 0);
		uint32_t count = strtoul(argv[1], NULL, 0);
		writeHexadecimalTable((void*) address, count);
	}
}

static void actionSystemBusyWait(void) {
	myosSystemBusyWait();
}

static void actionReadLink(int argc, const char** argv) {
	if (argc > 1) {
		const char* path = argv[0];
		int bufferSize = PATH_MAX;
		void* buffer = malloc(bufferSize);
		ssize_t result = readlink(path, buffer, bufferSize);
		if (result > 0) {
			write(STDOUT_FILENO, buffer, result);
			printf("\n");
		} else {
			perror(NULL);
		}
		free(buffer);
	}
}

static void actionDebugKernel(int argc, const char** argv) {
	if (argc >= 1) {
		const char* kernelModuleName = argv[0];
		if (myosLogKernelModuleDebugReport(kernelModuleName)) {
			perror(NULL);
		}
	}
}

static void actionWriteFileContent(int argc, const char** argv) {
	if (argc == 3) {
		const char* path = argv[0];
		off_t offset = atoi(argv[1]);
		const char* string = argv[2];

		int fileDescriptorIndex = open(path, O_CREAT | O_WRONLY);
		if (fileDescriptorIndex != -1) {
			if (lseek(fileDescriptorIndex, offset, offset >= 0 ? SEEK_SET : SEEK_END) == -1) {
				perror(NULL);

			} else {
				ssize_t count = write(fileDescriptorIndex, string, strlen(string));
				if (count == -1) {
					perror(NULL);
				} else {
					printf("%d bytes written\n", count);
				}
			}

			close(fileDescriptorIndex);

		} else {
			perror(NULL);
		}
	}
}

static void actionAppendFileContent(int argc, const char** argv) {
	if (argc == 2) {
		const char* filePath = argv[0];
		int fileDescriptorIndex = open(filePath, O_WRONLY | O_APPEND);
		if (fileDescriptorIndex != -1) {
			ssize_t count = write(fileDescriptorIndex, argv[1], strlen(argv[1]));
			if (count == -1) {
				perror(NULL);
			}
			close(fileDescriptorIndex);

		} else {
			perror(NULL);
		}
	}
}

static void actionTime(int argc, const char** argv) {
	time_t timeInstance = time(NULL);
	printf("%d - %s\n", timeInstance, ctime(&timeInstance));
}

static void actionFlushCache(int argc, const char** argv) {
	myosFlushCache();
}

static void actionFlushAndClearCache(int argc, const char** argv) {
	myosFlushAndClearCache();
}

static int compare(struct DoubleLinkedListElement* doubleLinkedListElement1, struct DoubleLinkedListElement* doubleLinkedListElement2) {
	struct Command* command1 = (void*) doubleLinkedListElement1;
	struct Command* command2 = (void*) doubleLinkedListElement2;

	return strcmp(command1->id, command2->id);
}

static void actionCompare(int argc, const char** argv) {
	if (argc == 2) {
		bool error = false;
		bool equal = true;

		const char* path1 = argv[0];
		const char* path2 = argv[1];

		const int bufferSize = 1024;
		void* buffer1 = malloc(bufferSize);
		void* buffer2 = malloc(bufferSize);

		int fileDescriptorIndex1 = open(path1, O_RDONLY);
		if (fileDescriptorIndex1 != -1) {
			if (lseek(fileDescriptorIndex1, 0, SEEK_SET) == -1) {
				perror(NULL);
				error = true;

			} else {
				int fileDescriptorIndex2 = open(path2, O_RDONLY);
				if (fileDescriptorIndex2 != -1) {
					if (lseek(fileDescriptorIndex2, 0, SEEK_SET) == -1) {
						perror(NULL);
						error = true;

					} else {
						while (equal && !error) {
							ssize_t count1 = read(fileDescriptorIndex1, buffer1, bufferSize);
							if (count1 == -1) {
								perror(NULL);
								error = true;
								break;
							}
							ssize_t count2 = read(fileDescriptorIndex2, buffer2, bufferSize);
							if (count2 == -1) {
								perror(NULL);
								error = true;
								break;
							}

							if (count1 != count2){
								equal = false;
								break;

							} else if (count1 == 0) {
								assert(count2 == 0);
								break;

							} else {
								equal = memcmp(buffer1, buffer2, count1) == 0;
							}
						}
					}
				} else {
					perror(NULL);
					error = true;
				}
				close(fileDescriptorIndex2);
			}
			close(fileDescriptorIndex1);

		} else {
			perror(NULL);
			error = true;
		}

		free(buffer1);
		free(buffer2);

		if (!error) {
			if (equal) {
				printf("Files are equal!\n");
			} else {
				printf("Files are different!\n");
			}
		}
	}
}

static void commonCopy(int argc, const char** argv, bool slow) {
	if (2 <= argc) {
		const char* path1 = argv[0];
		const char* path2 = argv[1];

		const int bufferSize = 1024;
		void* buffer1 = malloc(bufferSize);
		void* buffer2 = malloc(bufferSize);

		int fileDescriptorIndex1 = open(path1, O_RDONLY);
		if (fileDescriptorIndex1 != -1) {
			int fileDescriptorIndex2 = open(path2, O_RDWR | O_CREAT | O_TRUNC);
			if (fileDescriptorIndex2 != -1) {
				if (lseek(fileDescriptorIndex2, 0, SEEK_SET) == -1) {
					perror(NULL);
				} else {
					while (true) {
						ssize_t count = read(fileDescriptorIndex1, buffer1, bufferSize);
						if (count == -1) {
							perror(NULL);
							break;
						} else if (count == 0) {
							break;
						} else {
							if (count != write(fileDescriptorIndex2, buffer1, count)) {
								perror(NULL);
								break;
							}
#ifndef NDEBUG
							if (lseek(fileDescriptorIndex2, -count, SEEK_CUR) == -1) {
								perror(NULL);
								break;
							}
							if (count != read(fileDescriptorIndex2, buffer2, count)) {
								perror(NULL);
								break;
							}
							if (memcmp(buffer1, buffer2, count) != 0) {
								printf("%u %u\n", lseek(fileDescriptorIndex1, 0, SEEK_CUR), lseek(fileDescriptorIndex2, 0, SEEK_CUR));
								writeHexadecimalTable(buffer1, count);
								writeHexadecimalTable(buffer2, count);
								assert(false);
							}
#endif
						}

						if (slow) {
							sleep(1);
						}
					}
				}

			} else {
				perror(NULL);
			}
			close(fileDescriptorIndex1);
			close(fileDescriptorIndex2);

		} else {
			perror(NULL);
		}

		free(buffer1);
		free(buffer2);
	}
}

static void actionCopy(int argc, const char** argv) {
	commonCopy(argc, argv, false);
}

static void actionSlowCopy(int argc, const char** argv) {
	commonCopy(argc, argv, true);
}

static void actionChangeFileSize(int argc, const char** argv) {
	off_t newSize;
	if (argc >= 2 && 2 == sscanf(argv[1], "%d", &newSize)) {
		const char* path = argv[0];
		if(truncate(path, newSize) == -1) {
			perror(NULL);
		}
	}
}

static void actionCreateEmptyFile(int argc, const char** argv) {
	if (argc >= 1) {
		const char* path = argv[0];
		int fileDescriptorIndex = open(path, O_CREAT | O_WRONLY);
		if (fileDescriptorIndex < 0) {
			perror(NULL);
		} else {
			close(fileDescriptorIndex);
		}
	}
}

static void actionCreateSymbolicLink(int argc, const char** argv) {
	if (argc >= 2) {
		const char* targetPath = argv[0];
		const char* symbolicLinkPathToCreate = argv[1];
		if (symlink(targetPath, symbolicLinkPathToCreate) != 0) {
			perror(NULL);
		}
	}
}

static void actionCreateEmptyFileWithBigName(int argc, const char** argv) {
	const int nameLength = 128;
	char name[nameLength];
	sprintf(name, "%d", time(NULL));
	memset(name + strlen(name), 'X', nameLength - strlen(name) - 1);
	name[nameLength - 1] = '\0';

	int fileDescriptorIndex = open(name, O_CREAT | O_WRONLY);
	if (fileDescriptorIndex < 0) {
		perror(NULL);
	} else {
		close(fileDescriptorIndex);
	}
}

static void actionCreateDirectory(int argc, const char** argv) {
	if (argc > 0) {
		const char* path = argv[0];
		if (mkdir(path, 0) != 0) {
			perror(NULL);
		}
	}
}

static void actionPrintTerminalSettings(int argc, const char** argv) {
	struct termios termiosInstance;
	if (-1 != tcgetattr(STDOUT_FILENO, &termiosInstance)) {
		printf("c_iflag=%X\n", termiosInstance.c_iflag);
		printf("c_oflag=%X\n", termiosInstance.c_oflag);
		printf("c_cflag=%X\n", termiosInstance.c_cflag);
		printf("c_lflag=%X\n", termiosInstance.c_lflag);
	} else {
		perror(NULL);
	}
}

static void setCursorVisibility(int argc, const char** argv, bool visibility) {
	if (argc > 0) {
		char* devicePath = strdup(argv[0]);

		if (strlen(devicePath) != 0) {
			int fileDescriptorIndex = open(stringUtilsTrim(devicePath), O_WRONLY);
			if (fileDescriptorIndex != -1) {
				dprintf(fileDescriptorIndex, "\x1B[?25%c", visibility ? 'h': 'l');
			} else {
				perror(NULL);
			}

			close(fileDescriptorIndex);
		} else {
			printf("\x1B[?25%c", visibility ? 'h': 'l');
		}

		free(devicePath);
	}
}

static void actionHideCursor(int argc, const char** argv) {
	setCursorVisibility(argc, argv, false);
}

static void actionShowCursor(int argc, const char** argv) {
	setCursorVisibility(argc, argv, true);
}

static void setAlternativeScreenBufferEnabled(int argc, const char** argv, bool enabled) {
	if (argc > 0) {
		int fileDescriptorIndex = open(argv[0], O_WRONLY);
		if (fileDescriptorIndex != -1) {
			dprintf(fileDescriptorIndex, "\x1B[?1049%c", enabled ? 'h': 'l');
		} else {
			perror(NULL);
		}

		close(fileDescriptorIndex);
	} else {
		printf("\x1B[?1049%c", enabled ? 'h': 'l');
	}
}

static void actionSetCursorPosition(int argc, const char** argv) {
	if (argc >= 3) {
		const char* devicePath = argv[0];
		int row = atoi(argv[1]);
		int column = atoi(argv[2]);

		int fileDescriptorIndex = open(devicePath, O_WRONLY);
		if (fileDescriptorIndex != -1) {
			dprintf(fileDescriptorIndex, "\x1B[%d;%dH", row, column);
			close(fileDescriptorIndex);
		} else {
			perror(NULL);
		}
	}
}

static void actionCommonClear(int argc, const char** argv, bool screen) {
	if (1 <= argc && argc <= 2) {
		const char* devicePath = NULL;
		int parameter = atoi(argv[0]);
		if (argc == 2) {
			devicePath = argv[1];
		}

		int fileDescriptorIndex = STDOUT_FILENO;
		if (devicePath != NULL) {
			fileDescriptorIndex = open(devicePath, O_WRONLY);
		}
		if (fileDescriptorIndex != -1) {
			dprintf(fileDescriptorIndex, "\x1B[%d%c", parameter, (screen ? 'J' : 'K'));
		} else {
			perror(NULL);
		}
		if (devicePath != NULL) {
			close(fileDescriptorIndex);
		}
	}
}

static void actionClearLine(int argc, const char** argv) {
	actionCommonClear(argc, argv, false);
}

static void actionClearScreen(int argc, const char** argv) {
	actionCommonClear(argc, argv, true);
}

static void actionSwitchToAlternativeScreenBuffer(int argc, const char** argv) {
	setAlternativeScreenBufferEnabled(argc, argv, true);
}

static void actionSwitchFromAlternativeScreenBuffer(int argc, const char** argv) {
	setAlternativeScreenBufferEnabled(argc, argv, false);
}

static void actionCommonSaveRestoreCursor(int argc, const char** argv, bool save) {
	int fileDescriptorIndex = STDOUT_FILENO;
	if (argc == 1) {
		fileDescriptorIndex = open(argv[0], O_WRONLY);
	}
	if (fileDescriptorIndex != -1) {
		dprintf(fileDescriptorIndex, "\x1B%c", (save ? '6' : '7'));
	} else {
		perror(NULL);
	}
	if (argc == 1) {
		close(fileDescriptorIndex);
	}
}

static void actionSaveCursor(int argc, const char** argv) {
	actionCommonSaveRestoreCursor(argc, argv, true);
}

static void actionRestoreCursor(int argc, const char** argv) {
	actionCommonSaveRestoreCursor(argc, argv, false);
}

static void printSegmentInformation(FILE* file, const char* segmentName, struct ProcessMemorySegmentLimits* processMemorySegmentLimits) {
	fprintf(file, "%s:\n", segmentName);
	if (!processMemorySegmentLimits->isEmpty) {
		fprintf(file, "  limits: from %X to %X (inclusive)\n", processMemorySegmentLimits->firstAddress, processMemorySegmentLimits->lastAddress);
	} else {
		fprintf(file, "  (empty)\n");
	}
}

static void actionGetSegmentsLimits(int argc, const char** argv) {
	struct ProcessMemorySegmentsLimits processMemorySegmentsLimits;
	if (myosGetProcessMemorySegmentsLimits(&processMemorySegmentsLimits)) {
		perror(NULL);
	} else {
		printSegmentInformation(stdout, "code", &processMemorySegmentsLimits.code);
		printSegmentInformation(stdout, "data", &processMemorySegmentsLimits.data);
		printSegmentInformation(stdout, "stack", &processMemorySegmentsLimits.stack);
	}
}

static void actionPrintRandom(int argc, const char** argv) {
	int number;
	__asm__ __volatile__(
		"rdrand %%eax;"
		: "=a"(number)
		:
		:);

	printf("%8.d\n", number);
}

static void actionListEnvironmentParameters(int argc, const char** argv) {
	char* environmentParameter;
	int i = 0;
	while ((environmentParameter = environ[i++]) != NULL) {
		printf("%s\n", environmentParameter);
	}
}

static void registerCommands(struct DoubleLinkedList* commands) {
	struct Command* command;

	command = malloc(sizeof(struct Command));
	command->id = "listEnv";
	command->action = &actionListEnvironmentParameters;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "rand";
	command->action = &actionPrintRandom;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "getSegmentsLimits";
	command->action = &actionGetSegmentsLimits;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "saveCursor";
	command->action = &actionSaveCursor;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "restoreCursor";
	command->action = &actionRestoreCursor;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "stty";
	command->action = &actionPrintTerminalSettings;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "rmdir";
	command->action = &actionReleaseDirectory;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "link";
	command->action = &actionCreateName;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "unlink";
	command->action = &actionReleaseName;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "mkdir";
	command->action = &actionCreateDirectory;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "touch2";
	command->action = &actionCreateEmptyFileWithBigName;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "touch";
	command->action = &actionCreateEmptyFile;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "symlink";
	command->action = &actionCreateSymbolicLink;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "truncate";
	command->action = &actionChangeFileSize;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "cmp";
	command->action = &actionCompare;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "cp";
	command->action = &actionCopy;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "slowCp";
	command->action = &actionSlowCopy;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "flushAndClearCache";
	command->action = &actionFlushAndClearCache;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "append";
	command->action = &actionAppendFileContent;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "flushCache";
	command->action = &actionFlushCache;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "time";
	command->action = &actionTime;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "slowLs";
	command->action = &actionSlowList;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "assertFalse";
	command->action = &actionAssertFalse;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "recursiveSignalHandler";
	command->action = &actionRecursiveSignalHandler;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "memoryRead";
	command->action = &actionReadFromAddress;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "reboot";
	command->action = &actionReboot;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "abort";
	command->action = &actionAbort;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "invalidSystemCall";
	command->action = (void (*)(int, const char**)) &myosInvalidSystemCall;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "help";
	command->action = &actionHelp;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "raiseSIGSEGV";
	command->action = &actionRaiseSIGSEGV;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "systemAssertFalse";
	command->action = &actionSystemAssertFalse;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "raiseSIGWINCH";
	command->action = &actionRaiseSIGWINCH;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "kill";
	command->action = &actionSendSignal;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "quit";
	command->action = &actionQuit;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "stressCPU";
	command->action = &actionStressCPU;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "forceSIGSEGV";
	command->action = &actionForceSIGSEGV;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "seekStdout";
	command->action = &actionSeekStdout;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "forceKernelSIGSEGV";
	command->action = &actionForceKernelSIGSEGV;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "forceDivisionByZero";
	command->action = &actionForceDivisionByZero;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "forceKernelSIGILL";
	command->action = &actionForceKernelSIGILL;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "sleep";
	command->action = &actionSleep;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "infinitySignalHandler";
	command->action = &actionInfinitySignalHandler;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "pwd";
	command->action = &actionGetCWD;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "cd";
	command->action = &actionSetCWD;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "cat";
	command->action = &actionReadTextFileContent;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "dump";
	command->action = &actionReadBinaryFileContent;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "dumpMemory";
	command->action = &actionReadMemoryContent;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "stat";
	command->action = &actionStat;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "lstat";
	command->action = &actionLStat;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "systemBusyWait";
	command->action = (void (*)(int, const char**)) &actionSystemBusyWait;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "readlink";
	command->action = &actionReadLink;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "debugKernel";
	command->action = &actionDebugKernel;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "write";
	command->action = &actionWriteFileContent;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "hideCursor";
	command->action = &actionHideCursor;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "showCursor";
	command->action = &actionShowCursor;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "toAlternativeScreenBuffer";
	command->action = &actionSwitchToAlternativeScreenBuffer;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "fromAlternativeScreenBuffer";
	command->action = &actionSwitchFromAlternativeScreenBuffer;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "clear";
	command->action = &actionClearScreen;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "clearLine";
	command->action = &actionClearLine;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "setCursorPosition";
	command->action = &actionSetCursorPosition;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "describeOpenFileDescription";
	command->action = &actionDescribeOpenFileDescription;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "testRandom";
	command->action = &actionTestRandom;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "testPoll";
	command->action = &actionTestPoll;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "testPoll2";
	command->action = &actionTestPoll2;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "testPoll3";
	command->action = &actionTestPoll3;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "testPoll4";
	command->action = &actionTestPoll4;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "system";
	command->action = &actionSystem;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "rename";
	command->action = &actionRename;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "stressGlob";
	command->action = &actionStressGlob;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	command = malloc(sizeof(struct Command));
	command->id = "malloc";
	command->action = &actionMalloc;
	doubleLinkedListInsertAfterLast(commands, &command->doubleLinkedListElement);

	doubleLinkedListSort(commands, &compare);
}

static struct winsize winsizeInstance;

static void clearCommandLine(void) {
	int totalLength = strlen(PROMPT) + commandLineLength;
	int rowCount = mathUtilsMax(0, mathUtilsCeilOfUint32Division(totalLength, winsizeInstance.ws_col) - 1);

	while (true) {
		printf("\x1B[2K");
		rowCount--;
		if (rowCount >= 0) {
			printf("\x1B[A");
		} else {
			break;
		}
	}
	printf("\x1B[80D");
}

// TODO: It's assuming that the line begins at column number 0 which isn't always true
static void redrawCommandLine(int oldCommandLineLength) {
	int totalLength = strlen(PROMPT) + oldCommandLineLength;
	int rowCount = mathUtilsCeilOfUint32Division(totalLength, winsizeInstance.ws_col);
	int lastRowColumnCount = totalLength % winsizeInstance.ws_col;

	if (lastRowColumnCount == 0 && oldCommandLineLength > 1) {
		printf("\x1B[A");
		rowCount--;
	}
	printf("\x1B[2K");

	if (rowCount > 1) {
		printf("\x1B[%dA", rowCount - 1);
	}
	printf("\x1B[%dD", winsizeInstance.ws_col);

	printf("%s%s", PROMPT, commandLine);
}

static void executeCommand(const char* commandId, int argc, const char** argv) {
	struct Command* command = NULL;

	if (commandId != NULL) {
		struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListFirst(&commands);
		while (doubleLinkedListElement != NULL) {
			command = (void*) doubleLinkedListElement;
			doubleLinkedListElement = doubleLinkedListElement->next;

			assert(command->id != NULL);
			assert(command->action != NULL);

			if (strcmp(commandId, command->id) == 0) {
				break;
			} else {
				command = NULL;
			}
		}
	}

	if (command != NULL) {
		command->action(argc, argv);
	} else {
		printf("Unknown command (%s)!\n", commandId);
	}
}

static struct termios originalTTYConfiguration;
static void restoreTTYConfiguration(void) {
	tcsetattr(STDIN_FILENO, TCSANOW, &originalTTYConfiguration);
}

int main(int argc, const char** argv) {
	bool interactive = argc <= 1;

	doubleLinkedListInitialize(&commands);
	registerCommands(&commands);
	srand((unsigned int) time(NULL));

	struct sigaction act;
	act.sa_flags = 0;
	act.sa_handler = &handleSignal;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGUSR2, &act, NULL)) {
		perror(NULL);
	}

	act.sa_flags = 0;
	act.sa_handler = &handleSignal;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGHUP, &act, NULL)) {
		perror(NULL);
	}

	act.sa_flags = SA_NODEFER;
	act.sa_handler = &handleSignal;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGUSR2);
	if (sigaction(SIGUSR1, &act, NULL)) {
		perror(NULL);
	}

	act.sa_flags = 0;
	act.sa_handler = &handleSignal;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGSYS, &act, NULL)) {
		perror(NULL);
	}

	if (interactive) {
		setvbuf(stderr, NULL, _IONBF, 0);
		setvbuf(stdout, NULL, _IONBF, 0);

		act.sa_flags = 0;
		act.sa_handler = &handleSignal;
		sigemptyset(&act.sa_mask);
		if (sigaction(SIGSEGV, &act, NULL)) {
			perror(NULL);
		}

		act.sa_flags = 0;
		act.sa_handler = &handleSignal;
		sigemptyset(&act.sa_mask);
		if (sigaction(SIGTSTP, &act, NULL)) {
			perror(NULL);
		}

		act.sa_flags = 0;
		act.sa_handler = &handleSignal;
		sigemptyset(&act.sa_mask);
		if (sigaction(SIGCONT, &act, NULL)) {
			perror(NULL);
		}

		act.sa_flags = 0;
		act.sa_handler = &handleSignal;
		sigemptyset(&act.sa_mask);
		if (sigaction(SIGABRT, &act, NULL)) {
			perror(NULL);
		}

		sigset_t signalSet;
		sigemptyset(&signalSet);
		sigaddset(&signalSet, SIGUSR2);
		sigaddset(&signalSet, SIGABRT);
		sigaddset(&signalSet, SIGSTOP);
		if (sigprocmask(SIG_BLOCK, &signalSet, NULL)) {
			perror(NULL);
		}

		if (-1 == tcgetattr(STDIN_FILENO, &originalTTYConfiguration)) {
			perror(NULL);

		} else {
			struct termios termiosInstance;
			memcpy(&termiosInstance, &originalTTYConfiguration, sizeof(struct termios));
			termiosInstance.c_lflag = termiosInstance.c_lflag & ~ICANON & ~ECHO;
			if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &termiosInstance)) {
				perror(NULL);
			} else {
				atexit(&restoreTTYConfiguration);
			}
		}

		if (-1 == ioctl(1, TIOCGWINSZ, &winsizeInstance)) {
			perror(NULL);
		}

		struct TTYUtilsKeyEscapeSequenceParsingContext context;
		ttyUtilsInitializeKeyEscapeSequenceParsingContext(&context);

		doubleLinkedListInitialize(&historicalCommandLinesList);

		printf("This is the system call tester! Note it's NOT a shell!\nType \"help\" to see available options...\n");
		printf(PROMPT);

		bool quit = false;
		while (!quit) {
			char character;
			ssize_t count = read(STDIN_FILENO, &character, sizeof(char));
			if (count == 1) {
				int key = ttyUtilsParseKeyEscapeSequence(&context, character, false);
				if (key != NO_KEY_AVAILABLE_YET) {
					if (key == KEY_UP || key == KEY_DOWN) {
						struct CommandLineHistory* historicalCommandLine = NULL;
						if (lastHistoricalCommandLineRestored != NULL) {
							if (key == KEY_UP) {
								if (lastHistoricalCommandLineRestored->previous != NULL) {
									lastHistoricalCommandLineRestored = lastHistoricalCommandLineRestored->previous;
									historicalCommandLine = (void*) lastHistoricalCommandLineRestored;
								}

							} else {
								assert(key == KEY_DOWN);
								if (lastHistoricalCommandLineRestored->next != NULL) {
									lastHistoricalCommandLineRestored = lastHistoricalCommandLineRestored->next;
									historicalCommandLine = (void*) lastHistoricalCommandLineRestored;
								}
							}

						} else if (key == KEY_UP && doubleLinkedListSize(&historicalCommandLinesList) > 0) {
							lastHistoricalCommandLineRestored = doubleLinkedListLast(&historicalCommandLinesList);
							historicalCommandLine = (void*) lastHistoricalCommandLineRestored;
						}

						if (historicalCommandLine != NULL) {
							clearCommandLine();

							strcpy(commandLine, historicalCommandLine->commandLine);
							commandLineLength = strlen(commandLine);

							printf("%s%s", PROMPT, commandLine);

						}

					} else if (key == 127) {
						if (commandLineLength > 0) {
							commandLineLength--;
							commandLine[commandLineLength] = '\0';

							redrawCommandLine(commandLineLength + 1);
						}

					} else {
						if (32 <= key && key < 127) {
							if (commandLineLength + 1 <= COMMAND_LINE_MAX_LENGTH ) {
								commandLine[commandLineLength++] = (char) key;
								printf("%c", key);
							}


						} else if (key == '\n') {
							printf("\n");

							commandLine[commandLineLength++] = '\0';

							if (doubleLinkedListSize(&historicalCommandLinesList) + 1 > MAX_HISTORICAL_COMMAND_LINES) {
								free(doubleLinkedListRemoveFirst(&historicalCommandLinesList));
							}

							bool insertOnHistory = false;
							struct CommandLineHistory* commandLineHistory;
							if (doubleLinkedListSize(&historicalCommandLinesList) > 0) {
								struct CommandLineHistory* lastHistoricalCommandLine = (void*) doubleLinkedListLast(&historicalCommandLinesList);
								insertOnHistory = strcmp(commandLine, lastHistoricalCommandLine->commandLine) != 0;
							} else {
								insertOnHistory = true;
							}

							if (insertOnHistory) {
								commandLineHistory = malloc(sizeof(struct CommandLineHistory));
								strcpy(commandLineHistory->commandLine, commandLine);
								doubleLinkedListInsertAfterLast(&historicalCommandLinesList, &commandLineHistory->doubleLinkedListElement);
							}
							lastHistoricalCommandLineRestored = NULL;

							const size_t ARGUMENTS_BUFFER_SIZE = 4096;
							char argumentsBuffer[ARGUMENTS_BUFFER_SIZE];
							const size_t ARGUMENTS_LENGTH = 128;
							const char* arguments[ARGUMENTS_LENGTH];
							int argumentCount;

							commandLineUtilsSplit(commandLine, argumentsBuffer, ARGUMENTS_BUFFER_SIZE,
									arguments, ARGUMENTS_LENGTH, &argumentCount);

							if (argumentCount > 0) {
								executeCommand(arguments[0], argumentCount - 1, argumentCount > 1 ? &arguments[1] : NULL);
							}

							commandLineLength = 0;

							printf("$> ");
						}
					}
				}
			}
		}

		printf("Bye!!!\n");

	} else {
		assert(argc >= 2);
		executeCommand(argv[1], argc - 2, argc > 2 ? &argv[2] : NULL);
	}
	return EXIT_SUCCESS;
}
