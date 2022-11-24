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
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <utime.h>

#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <myos.h>

#include "kernel/limits.h"
#include "kernel/system_calls.h"

#include "user/util/dynamic_array_utils.h"
#include "user/util/dynamic_array.h"
#include "user/util/scanner.h"
#include "user/util/wildcard_path_pattern_matcher.h"
#include "user/util/wildcard_pattern_matcher.h"

#include "util/formatter.h"
#include "util/path_utils.h"
#include "util/scanner.h"
#include "util/string_stream_writer.h"
#include "util/string_utils.h"

#define STR(x) #x
#define XSTR(x) STR(x)

int errno;

static void (*atExitCallbacks[ATEXIT_MAX])();
static int atExitCallbacksCount = 0;

void closeAllStreams();

static char* calculateExecutablePath(const char* executableName) {
	char* executablePath = NULL;
	char* buffer = NULL;
	char* path = getenv("PATH");

	if (path == NULL) {
		size_t bufferSize = confstr(_CS_PATH, NULL, 0);
		buffer = malloc(sizeof(char) * (bufferSize + 1));
		confstr(_CS_PATH, buffer, bufferSize);
		path = buffer;
	} else {
		buffer = strdup(path);
		path = buffer;
	}

	if (path != NULL) {
		static char* context;
		char* token = NULL;
		do {
			token = strtok_r(token == NULL ? path : NULL, ":", &context);
			if (token != NULL) {
				char buffer[PATH_MAX_LENGTH];
				struct stat statInstance;
				struct StringStreamWriter stringStreamWriter;

				stringStreamWriterInitialize(&stringStreamWriter, buffer, PATH_MAX_LENGTH);
				streamWriterWriteString(&stringStreamWriter.streamWriter, token, UINT_MAX);
				streamWriterWriteString(&stringStreamWriter.streamWriter, "/", UINT_MAX);
				streamWriterWriteString(&stringStreamWriter.streamWriter, executableName, UINT_MAX);
				stringStreamWriterForceTerminationCharacter(&stringStreamWriter);

				int result = stat(buffer, &statInstance);
				if (result == 0) {
					executablePath = strdup(buffer);
					break;
				}
			}
		} while (token != NULL);
	}

	if (buffer != NULL) {
		free(buffer);
	}

	return executablePath;
}

void perror(const char *string) {
	int errnoCopy = errno;
	if (string != NULL && string[0] != '\0') {
		fprintf(stderr, "%s: ", string);
	}
	if (0 <= errnoCopy && errnoCopy < sys_nerr) {
		fprintf(stderr, "%s\n", sys_errlist[errnoCopy]);
	}
}

void* sbrk(int increment) {
	int result;
	void* sbrkResult;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(sbrkResult)
		: "a"(SYSTEM_CALL_CHANGE_DATA_SEGMENT_SIZE), "b"(increment)
		: "memory");
	if (result) {
		errno = result;
		return (void*)-1;
	} else {
		return sbrkResult;
	}
}

pid_t getpid(void) {
	pid_t result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_GET_PROCESS_ID)
		: "memory");
	return result;
}

pid_t getppid(void) {
	pid_t result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_GET_PARENT_PROCESS_ID)
		: "memory");
	return result;
}

pid_t fork(void) {
	return myosForkAndGenerateSignal(0);
}

void __attribute__ ((cdecl)) exit(int exitStatus) {
	for (int i = 0; i < atExitCallbacksCount; i++) {
		void (*atExitCallback)() = atExitCallbacks[i];
		atExitCallback();
	}

	closeAllStreams();
	_exit(exitStatus);
}

void _exit(int exitStatus) {
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		:
		: "a"(SYSTEM_CALL_EXIT), "b"(exitStatus)
		: "memory");
}

unsigned int sleep(unsigned int seconds) {
	unsigned int result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_SLEEP), "b"(seconds * 1000)
		: "memory");
	return result;
}

int open(const char* path, int flags, ...) {
	int result;
	int fileDescriptorIndex;

	mode_t mode = 0;
	if ((flags & O_CREAT) != 0) {
		va_list ap;

		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}

	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(fileDescriptorIndex)
		: "a"(SYSTEM_CALL_OPEN), "b"(path), "c"(flags), "d"(mode)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return fileDescriptorIndex;
	}
}

int close(int fileDescriptorIndex) {
	int result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_CLOSE), "b"(fileDescriptorIndex)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

ssize_t read(int fileDescriptorIndex, void* buffer, size_t count) {
	int result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(count)
		: "a"(SYSTEM_CALL_READ), "b"(fileDescriptorIndex), "c"(buffer), "d"(count)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return count;
	}
}

ssize_t write(int fileDescriptorIndex, const void* buffer, size_t count) {
	int result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(count)
		: "a"(SYSTEM_CALL_WRITE), "b"(fileDescriptorIndex), "c"(buffer), "d"(count)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return count;
	}
}

pid_t wait(int *status) {
	return waitpid(-1, status, 0);
}

pid_t waitpid(pid_t scope, int* status, int options) {
	int result;
	int internalExitStatus;
	pid_t childProcessId;

	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(internalExitStatus), "=c"(childProcessId)
		: "a"(SYSTEM_CALL_WAIT), "b"(scope), "c"(options)
		: "memory");
	if (status != NULL) {
		*status = internalExitStatus;
	}
	if (result) {
		errno = result;
		return -1;
	} else {
		return childProcessId;
	}
}

char* program_invocation_name;
char* program_invocation_short_name;
void stdlibInitialize(char**);
void __attribute__ ((cdecl)) myosStandardLibraryInitialize(int argc, char** argv, char** environmentParameters) {
	stdlibInitialize(environmentParameters);

	if (argc > 0 && argv != NULL && argv[0] != NULL) {
		program_invocation_name = strdup(argv[0]);
		int i = strlen(argv[0]) - 1;
		while (true) {
			if (i >= 0 && argv[0][i] != '/') {
				i--;
			} else {
				program_invocation_short_name = strdup(&argv[0][i + 1]);
				break;
			}
		}

	} else {
		program_invocation_name = NULL;
		program_invocation_short_name = NULL;
	}
}

DIR* opendir(const char* path) {
	int fileDescriptorIndex = open(path, O_PATH | O_DIRECTORY);
	if (fileDescriptorIndex != -1) {
		DIR* result = malloc(sizeof(DIR));
		if (result != NULL) {
			result->fileDescriptorIndex = fileDescriptorIndex;
			return result;

		} else {
			close(fileDescriptorIndex);
			errno = ENOMEM;
			return NULL;
		}

	} else {
		return NULL;
	}
}
static struct dirent sharedDirentInstance;
struct dirent* readdir(DIR* directory) {
	int result;
	bool endOfDirectory;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(endOfDirectory)
		: "a"(SYSTEM_CALL_READ_DIRECTORY_ENTRY), "b"(directory->fileDescriptorIndex), "c"(&sharedDirentInstance)
		: "memory");
	if (result) {
		errno = result;
		return NULL;
	} else if (endOfDirectory) {
		return NULL;
	} else {
		return &sharedDirentInstance;
	}
}

int closedir(DIR* directory) {
	if (directory != NULL) {
		int result = close(directory->fileDescriptorIndex);
		free(directory);
		return result;

	} else {
		errno = EINVAL;
		return -1;
	}
}

int dirfd(DIR* directory) {
	if (directory != NULL) {
		return directory->fileDescriptorIndex;

	} else {
		errno = EINVAL;
		return -1;
	}
}

void rewinddir(DIR* directory) {
	if (directory != NULL) {
		__asm__ __volatile__(
			"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
			:
			: "a"(SYSTEM_CALL_REPOSITION_OPEN_FILE_DESCRIPTION_OFFSET), "b"(directory->fileDescriptorIndex), "c"(0), "d"(SEEK_SET)
			: "memory");
	}
}

time_t time(time_t* timeInstance) {
	time_t result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_GET_UNIX_TIME)
		: "memory");
	if (timeInstance != NULL) {
		*timeInstance = result;
	}
	return result;
}

int fstat(int fileDescriptorIndex, struct stat* statInstance) {
	int result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_STATUS), "b"(fileDescriptorIndex), "c"(statInstance)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int commonStatus(const char* path, struct stat* statInstance, int flags) {
	int result = -1;

	if (strcmp(path, "") == 0) {
		errno = ENOENT;

	} else {
		int fileDescriptorIndex = open(path, flags);
		if (fileDescriptorIndex != -1) {
			result = fstat(fileDescriptorIndex, statInstance);
			int errnoCopy = errno;
			close(fileDescriptorIndex);
			if (result) {
				errno = errnoCopy;
			}
		}
	}
	return result;
}

int stat(const char* path, struct stat* statInstance) {
	return commonStatus(path, statInstance, O_PATH);
}

int lstat(const char* path, struct stat* statInstance) {
	return commonStatus(path, statInstance, O_PATH | O_DO_NOT_FOLLOW_LAST_SYMBOLIC_LINK);
}

void doAssert(int condition, const char* format, ...) {
	if (!condition) {
		va_list ap;

		va_start(ap, format);
		vfprintf(stderr, format, ap);
		vfprintf(stderr, "\n", ap);
		va_end(ap);
		abort();
	}
}

int execv(const char* executablePath, char *const argv[]) {
	return execve(executablePath, argv, environ);
}

int execve(const char* executablePath, char* const argv[], char* const envp[]) {
	int result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_EXECUTE_EXECUTABLE), "b"(executablePath), "c"(argv), "d"(envp)
		: "memory");
	errno = result;
	return -1;
}

int execvpe(const char* executableNameOrPath, char* const argv[], char* const envp[]) {
	const char* executablePath = NULL;
	if (strchr(executableNameOrPath, '/') != NULL) {
		executablePath = executableNameOrPath;
	} else {
		executablePath = calculateExecutablePath(executableNameOrPath);
	}

	if (executablePath != NULL) {
		int result = execve(executablePath, argv, envp);
		if (executablePath != executableNameOrPath) {
			free((char*) executablePath);
		}
		return result;

	} else {
		errno = ENOENT;
		return -1;
	}
}

int execvp(const char* executableNameOrPath, char* const argv[]) {
	return execvpe(executableNameOrPath, argv, environ);
}

int execl(const char* executablePath, const char* arg, ...) {
	struct DynamicArray dynamicArray;

	dynamicArrayInitialize(&dynamicArray, sizeof(char*), NULL,
		(void (*)(void*, void*)) &dynamicArrayUtilsMemoryAllocatorRelease,
		(void* (*)(void*, void*, size_t)) &dynamicArrayUtilsMemoryAllocatorResize);

	va_list ap;

	va_start(ap, arg);

	const char* nextArgument = arg;
	while (true) {
		if (dynamicArrayInsertAfterLast(&dynamicArray, &nextArgument) == NULL) {
			errno = ENOMEM;
			return -1;
		}
		if (nextArgument != NULL) {
			nextArgument = va_arg(ap, char*);
		} else {
			break;
		}
	}

	va_end(ap);

	return execve(executablePath, dynamicArrayGetArray(&dynamicArray), environ);
}

off_t lseek(int fileDescriptorIndex, off_t offset, int whence) {
	int result;
	off_t resultingOffset;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(resultingOffset)
		: "a"(SYSTEM_CALL_REPOSITION_OPEN_FILE_DESCRIPTION_OFFSET), "b"(fileDescriptorIndex), "c"(offset), "d"(whence)
		: "memory");
	if (result) {
		errno = result;
		return (off_t) -1;
	} else {
		return (off_t) resultingOffset;
	}
}

void beforeCallSignalAction(int signalId, struct sigaction* act);
int sigaction(int signalId, const struct sigaction *act, struct sigaction *oldact) {
	int result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_CHANGE_SIGNAL_ACTION), "b"(&beforeCallSignalAction), "c"(signalId), "d"(act), "D"(oldact)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int kill(pid_t processId, int signalId) {
	int result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_GENERATE_SIGNAL), "b"(processId), "c"(signalId)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int killpg(int processGroupId, int signalId) {
	return kill(-processGroupId, signalId);
}

void __attribute__ ((cdecl)) callSignalAction(int signalId, struct sigaction* act, sigset_t* blockedSignalsForSignalDelivery) {
	assert(act->sa_handler != NULL);
	assert(act->sa_sigaction == NULL);

	act->sa_handler(signalId);
	if (sigprocmask(SIG_UNBLOCK, blockedSignalsForSignalDelivery, NULL)) {
		perror(NULL);
	}
}

int __attribute__ ((cdecl)) sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
	int result;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_CHANGE_SIGNALS_BLOCKAGE), "b"(how), "c"(set), "d"(oldset)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int raise(int signalId) {
	return kill(getpid(), signalId);
}

/*
 * The following code is an adaptation of the code proposed on figure 10.25 (implementation of the abort)
 * present on chapter 10 of the book Advanced Programming in the Unix Environment, third edition.
 */
void abort(void) {
	struct sigaction actionDefault;
	memset(&actionDefault, 0, sizeof(struct sigaction));
	sigemptyset(&actionDefault.sa_mask);
	actionDefault.sa_handler = SIG_DFL;

	/*
	 * Caller can't ignore SIGABRT, if so reset to default.
	 * After the adjustment the action can be the default or a custom handler execution.
	 */
	struct sigaction previousAction;
	sigaction(SIGABRT, NULL, &previousAction);
	if (previousAction.sa_handler == SIG_IGN) {
		sigaction(SIGABRT, &previousAction, NULL);
	}

	if (previousAction.sa_handler == SIG_IGN || previousAction.sa_handler == SIG_DFL) {
		/* Flush all open stdio streams since default action is to terminate the process. */
		closeAllStreams();
	}

	/* Unblock the SIGABRT signal. */
	sigset_t signalsSet;
	sigemptyset(&signalsSet);
	sigaddset(&signalsSet, SIGABRT);
	sigprocmask(SIG_UNBLOCK, &signalsSet, NULL);

	raise(SIGABRT);

	/* If we're here, process caught SIGABRT and returned. */

	/*
	 * Flush all open stdio streams. Note that the previous flush occurred only in case the action was the default.
	 * However, if that was the case, we wouldn't be here.
	 */
	closeAllStreams();

	/* It will terminate the process by restoring the default disposition for SIGABRT and then raising the signal for a second time. */
	sigaction(SIGABRT, &actionDefault, NULL);
	raise(SIGABRT);

	/* This should never be executed ... */
	exit(EXIT_FAILURE);
}

char* getcwd(char* buffer, size_t bufferSize) {
	int result = 0;

	if (buffer != NULL && bufferSize == 0) {
		result = EINVAL;

	} else {
		if (buffer == NULL) {
			if (bufferSize == 0) {
				bufferSize = PATH_MAX_LENGTH;
			}
			buffer = malloc(sizeof(char) * bufferSize);
			if (buffer == NULL) {
				result = ENOMEM;
			}
		}

		if (result == 0) {
		__asm__ __volatile__(
			"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
			: "=a"(result)
			: "a"(SYSTEM_CALL_GET_CURRENT_WORKING_DIRECTORY), "b"(buffer), "c"(bufferSize)
			: "memory");
		}
	}

	if (result) {
		errno = result;
		return NULL;
	} else {
		return buffer;
	}
}

int chdir(const char* path) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_SET_CURRENT_WORKING_DIRECTORY), "b"(path)
		: "memory");

	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

ssize_t readlink(const char* path, char* buffer, size_t bufferSize) {
	ssize_t result = -1;

	struct stat statInstance;
	if (lstat(path, &statInstance) == 0) {
		if (S_ISLNK(statInstance.st_mode)) {
			int fileDescriptorIndex = open(path, O_RDONLY | O_DO_NOT_FOLLOW_LAST_SYMBOLIC_LINK);
			if (fileDescriptorIndex >= 0) {
				result = read(fileDescriptorIndex, buffer, bufferSize);
				int errnoCopy = errno;
				close(fileDescriptorIndex);
				if (result) {
					errno = errnoCopy;
				}
			}

		} else {
			errno = EINVAL;
		}
	}

	return result;
}

int truncate(const char* path, off_t newSize) {
	int result;
	int fileDescriptorIndex = open(path, O_WRONLY);
	if (fileDescriptorIndex >= 0) {
		result = ftruncate(fileDescriptorIndex, newSize);
		int errnoCopy = errno;
		close(fileDescriptorIndex);
		if (result) {
			errno = errnoCopy;
		}
	} else {
		result = -1;
	}

	return result;
}

int ftruncate(int fileDescriptorIndex, off_t newSize) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_CHANGE_FILE_SIZE), "b"(fileDescriptorIndex), "c"(newSize)
		: "memory");

	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int mkdir(const char* newDirectoryPath, mode_t mode) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_CREATE_DIRECTORY), "b"(newDirectoryPath), "c"(mode)
		: "memory");

	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int mknod(const char* path, mode_t mode, dev_t device) {
	return 0;
}

int link(const char* targetPath, const char* namePathToCreate) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_CREATE_NAME), "b"(targetPath), "c"(namePathToCreate)
		: "memory");

	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int unlink(const char* namePathToRelease) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_RELEASE_NAME), "b"(namePathToRelease)
		: "memory");

	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int rmdir(const char* directoryPathToRelease) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_RELEASE_DIRECTORY), "b"(directoryPathToRelease)
		: "memory");

	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int symlink(const char* targetPath, const char* symbolicLinkPathToCreate) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_CREATE_SYMBOLIC_LINK), "b"(targetPath), "c"(symbolicLinkPathToCreate)
		: "memory");

	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int ioctl(int fileDescriptorIndex, unsigned long request, ...) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_CHANGE_DEVICE_PARAMETERS), "b"(fileDescriptorIndex), "c"(&request)
		: "memory");

	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

static int doSingleIntegerArgumentFcntl(int systemCallId, int fileDescriptorIndex, int command, int* argument) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(systemCallId), "b"(fileDescriptorIndex), "c"(&command)
		: "memory");
	return result;
}

int tcgetattr(int fileDescriptorIndex, struct termios* termiosInstance) {
	return ioctl(fileDescriptorIndex, TCGETS, termiosInstance);
}

speed_t cfgetispeed(const struct termios* termiosInstance) {
	// TODO: Implement me!
	return B230400;
}

int cfsetispeed(struct termios* termiosInstance, speed_t speed) {
	// TODO: Implement me!
	return 0;
}

speed_t cfgetospeed(const struct termios* termiosInstance) {
	// TODO: Implement me!
	return B230400;
}

int cfsetospeed(struct termios* termiosInstance, speed_t speed) {
	// TODO: Implement me!
	return 0;
}

int tcflush(int fileDescriptorIndex, int queueSelector) {
	return 0;
}

int tcsetattr(int fileDescriptorIndex, int optionalActions, const struct termios* termiosInstance) {
	int request;
	switch (optionalActions) {
		case TCSANOW:
			request = TCSETS;
			break;

		case TCSADRAIN:
			request = TCSETSW;
			break;

		case TCSAFLUSH:
			request = TCSETSF;
			break;

		default:
			return EINVAL;
			break;
	}

	return ioctl(fileDescriptorIndex, request, termiosInstance);
}

int atexit(void (*callback)(void)) {
	if (atExitCallbacksCount + 1 <= ATEXIT_MAX) {
		atExitCallbacks[atExitCallbacksCount++] = callback;
		return 0;
	} else {
		return 1;
	}
}

static int commonDuplicateFileDescriptor(int existentFileDescriptorIndex, int minimumFileDescriptorIndex, int newFileDescriptorIndex, int flags) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_DUPLICATE_FILE_DESCRIPTOR), "b"(existentFileDescriptorIndex), "c"(minimumFileDescriptorIndex),
		  	  "d"(&newFileDescriptorIndex), "D"(flags)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return newFileDescriptorIndex;
	}
}

int dup(int existentFileDescriptorIndex) {
	return commonDuplicateFileDescriptor(existentFileDescriptorIndex, 0, -1, 0);
}

int dup2(int existentFileDescriptorIndex, int newFileDescriptorIndex) {
	return commonDuplicateFileDescriptor(existentFileDescriptorIndex, 0, newFileDescriptorIndex, 0);
}

int fcntl(int fileDescriptorIndex, int command, ...) {
	if (command == F_DUPFD) {
		va_list ap;
		va_start(ap, command);
		int minimumFileDescriptorIndex = va_arg(ap, int);
		va_end(ap);
		return commonDuplicateFileDescriptor(fileDescriptorIndex, minimumFileDescriptorIndex, -1, 0);

	} else {
		int systemCallId = command == F_GETFD || command == F_SETFD
				? SYSTEM_CALL_CHANGE_FILE_DESCRIPTOR_PARAMETERS
				: SYSTEM_CALL_CHANGE_OPEN_FILE_DESCRIPTION_PARAMETERS;

		switch (command) {
			case F_GETFL:
			case F_GETOWN:
			case F_GETFD:
			{
				int argument;
				int result = doSingleIntegerArgumentFcntl(systemCallId, fileDescriptorIndex, command, &argument);
				if (result == 0) {
					return argument;
				} else {
					errno = result;
					return -1;
				}
			} break;

			default:
			{
				int result = 0;
				__asm__ __volatile__(
					"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
					: "=a"(result)
					: "a"(systemCallId), "b"(fileDescriptorIndex), "c"(&command)
					: "memory");
				if (result) {
					errno = result;
					return -1;
				} else {
					return 0;
				}
			} break;
		}
	}
}

int rename(const char* oldPath, const char* newPath) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_RENAME), "b"(oldPath), "c"(newPath)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

pid_t setsid(void) {
	int result = 0;
	pid_t sessionId = -1;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(sessionId)
		: "a"(SYSTEM_CALL_CREATE_SESSION_AND_PROCESS_GROUP)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return sessionId;
	}
}

pid_t getsid(pid_t processId) {
	int result = 0;
	pid_t sessionId = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(sessionId)
		: "a"(SYSTEM_CALL_GET_SESSION_ID), "b"(processId)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return sessionId;
	}
}

pid_t getpgid(pid_t processId) {
	int result = 0;
	pid_t processGroupId = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(processGroupId)
		: "a"(SYSTEM_CALL_GET_PROCESS_GROUP_ID), "b"(processId)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return processGroupId;
	}
}

pid_t getpgrp(void) {
	return getpgid(0);
}

int setpgid(pid_t processId, pid_t processGroupId) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(processGroupId)
		: "a"(SYSTEM_CALL_SET_PROCESS_GROUP), "b"(processId), "c"(processGroupId)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int setpgrp(void) {
	return setpgid(0, 0);
}

pid_t tcgetsid(int fileDescriptorIndex) {
	pid_t sessionId;
	if (ioctl(fileDescriptorIndex, TIOCGSID, &sessionId) == -1) {
		return -1;
	} else {
		return sessionId;
	}
}

int tcflow(int fileDescriptorIndex, int action) {
	// TODO: Implement me!
	return 0;
}

int tcsetpgrp(int fileDescriptorIndex, pid_t processGroupId) {
	if (ioctl(fileDescriptorIndex, TIOCSPGRP, &processGroupId) == -1) {
		return -1;
	} else {
		return 0;
	}
}

pid_t tcgetpgrp(int fileDescriptorIndex) {
	pid_t processGroupId;
	if (ioctl(fileDescriptorIndex, TIOCGPGRP, &processGroupId) == -1) {
		return -1;
	} else {
		return processGroupId;
	}
}

size_t confstr(int name, char* buffer, size_t bufferSize) {
	const char* value = NULL;
	switch (name) {
		case _CS_PATH:
			value = "/bin:/usr/bin";
			break;

		default:
			errno = EINVAL;
			return 0;
	}

	if (value != NULL) {
		if (buffer != NULL && bufferSize > 0) {
			return strlcpy(buffer, value, bufferSize) + 1;
		} else {
			return strlen(value) + 1;
		}
	} else {
		return 0;
	}
}

long fpathconf(int fileDescriptorIndex, int name) {
	switch (name) {
		case _PC_PIPE_BUF:
			return PIPE_BUF;

		case _PC_PATH_MAX:
			return PATH_MAX;

		default:
			errno = EINVAL;
			return -1;
	}
}

long pathconf(const char* path, int name) {
	return fpathconf(0, name);
}

int pipe(int pipeFileDescriptorIndexes[2]) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(pipeFileDescriptorIndexes[0]), "=c"(pipeFileDescriptorIndexes[1])
		: "a"(SYSTEM_CALL_CREATE_PIPE)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

int isatty(int fileDescriptorIndex) {
	struct stat statInstance;
	int result = fstat(fileDescriptorIndex, &statInstance);
	if (result == 0) {
		if (S_ISCHR(statInstance.st_mode)) {
			return 1;
		} else {
			errno = ENOTTY;
			return 0;
		}

	} else {
		return 0;
	}
}

int setrlimit(int resource, const struct rlimit* rlimitInstance) {
	return 0;
}

int getrlimit(int resource, struct rlimit* rlimitInstance) {
	switch (resource) {
		case RLIMIT_FSIZE:
			rlimitInstance->rlim_cur = FILE_MAX_SIZE;
			rlimitInstance->rlim_max = FILE_MAX_SIZE;
		break;
		case RLIMIT_DATA:
			rlimitInstance->rlim_cur = DATA_SEGMENT_MAX_SIZE;
			rlimitInstance->rlim_max = DATA_SEGMENT_MAX_SIZE;
		break;


	}
	return 0;
}

int access(const char* path, int mode) {
	 //TODO: There is more to be done here

	struct stat statInstance;
	int result = stat(path, &statInstance);
	if (result == 0) {
		return 0;
	} else {
		return result;
	}
}

int euidaccess(const char* path, int mode) {
	return access(path, mode);
}

long sysconf(int key) {
	switch(key) {
		case _SC_CLK_TCK:
			return 100;
		case _SC_PAGESIZE:
			return PAGE_FRAME_SIZE;

		default:
			errno = EINVAL;
			return -1;
	}
}

clock_t times(struct tms *processTimes) {
	return -1;
}

struct passwd* getpwnam(const char *name) {
	// TODO: Implement me!
	return NULL;
}

int glob(const char* pattern, int flags, int (*errorCallBack) (const char*, int), glob_t* globInstance) {
	return wildcardPathPatternMatcherMatch(pattern, flags, errorCallBack, globInstance);
}

void globfree(glob_t* globInstance) {
	wildcardPathPatternMatcherRelease(globInstance);
}

int fnmatch(const char* rawPattern, const char* string, int flags) {
	return wildcardPatternMatcherCompileAndMatch(rawPattern, string, flags);
}

int chmod(const char* path, mode_t mode) {
	return 0;
}

int chown(const char* path, uid_t owner, gid_t group) {
	return 0;
}

int lchown(const char* path, uid_t owner, gid_t group) {
	return 0;
}

int setreuid(uid_t ruid, uid_t euid) {
	// TODO: Implement me!
	return 0;
}

int setregid(gid_t rgid, gid_t egid) {
	// TODO: Implement me!
	return 0;
}

sighandler_t signal(int signalId, sighandler_t handler) {
	struct sigaction sigactionInstance;
	struct sigaction oldSigactionInstance;

	memset(&sigactionInstance, 0, sizeof(struct sigaction));
	sigactionInstance.sa_handler = handler;

	if (signalId != SIGALRM) {
		sigactionInstance.sa_flags |= SA_RESTART;
	}

	if (sigaction(signalId, &sigactionInstance, &oldSigactionInstance) == -1) {
		return SIG_ERR;
	} else {
		return oldSigactionInstance.sa_handler;
	}
}

int gettimeofday(struct timeval* tv, struct timezone* tz) {
	int result = 0;
	if (tv != NULL) {
		time_t timeResult = time(NULL);
		if (timeResult != -1) {
			tv->tv_sec = timeResult;
			tv->tv_usec = 0;
		} else {
			result = -1;
		}
	}
	if (tz != NULL) {
		memset(tz, 0, sizeof(struct timezone));
	}

	return result;
}

int poll(struct pollfd* ioEventMonitoringContexts, nfds_t ioEventMonitoringContextCount, int timeout) {
	int result;
	int triggeredEventsCount;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(triggeredEventsCount)
		: "a"(SYSTEM_CALL_MONITOR_IO_EVENTS), "b"(ioEventMonitoringContexts), "c"(ioEventMonitoringContextCount), "d"(timeout)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return triggeredEventsCount;
	}
}

static struct passwd passwdInstance;
struct passwd* getpwuid(uid_t uid) {
	// TODO: Implement me!

	passwdInstance.pw_name = "root";
	passwdInstance.pw_uid = 0;
	passwdInstance.pw_gid = 0;
	passwdInstance.pw_dir = "/home/";
	passwdInstance.pw_shell = "/bin/sh";

	return &passwdInstance;
}

char ttyNameBuffer[PATH_MAX];
char* ttyname(int fileDescriptorIndex) {
	struct stat targetStatInstance;
	int result = fstat(fileDescriptorIndex, &targetStatInstance);
	if (result == 0 && S_ISCHR(targetStatInstance.st_mode)) {
		DIR* directory = opendir("/dev");
		if (directory != NULL) {
			struct dirent* entry;
			while ((entry = readdir(directory)) != NULL) {
				if (stringUtilsStartsWith(entry->d_name, "tty")) {
					sprintf(ttyNameBuffer, "/dev/%s", entry->d_name);

					struct stat statInstance;
					result = stat(ttyNameBuffer, &statInstance);
					if (result == 0) {
						if (targetStatInstance.st_dev == statInstance.st_dev && targetStatInstance.st_ino == statInstance.st_ino) {
							return ttyNameBuffer;
						}

					} else {
						break;
					}
				}
			}
			closedir(directory);
		}
	}

	return NULL;
}

int gethostname(char* buffer, size_t bufferSize) {
	// TODO: Implement me!
	if (bufferSize >= 5) {
		strcpy(buffer, "host");
		return 0;
	} else {
		return -1;
	}
}

int fsync(int fileDescriptorIndex) {
	// TODO: Implement me!
	return 0;
}

int fchmod(int fileDescriptorIndex, mode_t mode) {
	// TODO: Implement me!
	return 0;
}

int fchown(int fileDescriptorIndex, uid_t owner, gid_t group) {
	// TODO: Implement me!
	return 0;
}

char* nl_langinfo(int item) {
	// TODO: Implement me!
	return "";
}

int futimens(int fileDescriptorIndex, const struct timespec times[2]) {
	// TODO: Implement me!
	return 0;
}

char* setlocale(int category, const char *locale) {
	// TODO: Implement me!
	return NULL;
}

struct passwd* getpwent(void) {
	// TODO: Implement me!
	return NULL;
}

void endpwent(void) {
}

mode_t umask(mode_t mask) {
	mode_t previousMask;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(previousMask)
		: "a"(SYSTEM_CALL_SET_FILE_MODE_CREATION_MASK), "b"(mask)
		: "memory");
	return previousMask;
}

void* dlopen(const char* filename, int flags) {
	assert(false);
	// TODO: Implement me!
	return NULL;
}

int dlclose(void* handle) {
	// TODO: Implement me!
	assert(false);
	return 0;
}

void* dlsym(void* handle, const char* symbol) {
	// TODO: Implement me!
	assert(false);
	return NULL;
}

char* dlerror(void) {
	// TODO: Implement me!
	assert(false);
	return NULL;
}

ssize_t pread(int fileDescriptorIndex, void* buffer, size_t count, off_t offset) {
	assert(false);
	return -1;
}

int utime(const char* path, const struct utimbuf* utimbufInstance) {
	return 0;
}

unsigned alarm(unsigned seconds) {
	assert(false);
	return 0;
}

int getgroups(int size, gid_t groups[]) {
	assert(false);
	return -1;
}
