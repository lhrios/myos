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
#include <float.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include "user/util/dynamic_array.h"
#include "user/util/scanner.h"
#include "user/util/simple_memory_allocator.h"

#include "util/priority_queue.h"
#include "util/search_utils.h"

static const char* defaultEnviron[] = {NULL};
char** environ = (void*) defaultEnviron;
static char** lastAllocatedEnviron = NULL;

static void memoryAllocatorRelease(void* memoryAllocatorContext, void* pointer) {
	free(pointer);
}

static void* memoryAllocatorResize(void* memoryAllocatorContext, void* pointer, size_t size) {
	return realloc(pointer, size);
}

static void* safeSbrk(int increment) {
	void* result = sbrk(increment);
	if ((void*) -1 == result) {
		perror(NULL);
		abort();
	}
	return result;
}

void stdlibInitialize(char** environmentParameters) {
	if (environmentParameters != NULL) {
		environ = environmentParameters;
	}
	simpleMemoryAllocatorInitialize(&safeSbrk);
}

long int strtol(const char* string, char** startOfNotProcessedString, int base) {
	_Static_assert(sizeof(int32_t) == sizeof(long int), "Expecting \"int32_t\" and \"long int\" to be the same.");

	long int output;
	errno = scannerParseInt32(string, base, true, true, (const char**) startOfNotProcessedString, (int32_t*) &output);
	return output;
}

unsigned long strtoul(const char* string, char** startOfNotProcessedString, int base) {
	_Static_assert(sizeof(uint32_t) == sizeof(unsigned long), "Expecting \"uint32_t\" and \"unsigned long\" to be the same.");

	unsigned long result;
	errno = scannerParseInt32(string, base, true, true, (const char**) startOfNotProcessedString, (int32_t*) &result);
	return result;
}

int atoi(const char *string) {
	return (int) strtol(string, NULL, 10);
}

const char* strsignal(int signalId) {
	if (1 <= signalId && signalId <= NUMBER_OF_SIGNALS) {
		return sys_siglist[signalId];
	} else {
		return NULL;
	}
}

void* malloc(size_t size) {
	void* result = simpleMemoryAllocatorAcquire(size);
	assert(size == 0 || result != NULL);
	return result;
}

void* realloc(void* pointer, size_t size) {
	void* result = simpleMemoryAllocatorResize(pointer, size);
	assert(size == 0 || result != NULL);
	return result;
}

void free(void* pointer) {
	simpleMemoryAllocatorRelease(pointer);
}

void* calloc(size_t elementSize, size_t elementCount) {
	void* result = malloc(elementCount * elementSize);
	if (result != NULL) {
		memset(result, 0, elementCount * elementSize);
	}
	return result;
}

void qsort(void* array, size_t arrayLength, size_t elementSize, int (*comparator)(const void*, const void*)) {
	priorityQueueInplaceArraySort(array, arrayLength, elementSize, comparator, false, NULL);
}

void qsort_r(void* array, size_t arrayLength, size_t elementSize, int (*comparator)(const void*, const void*, void*), void* argument) {
	priorityQueueInplaceArraySort(array, arrayLength, elementSize, comparator, true, argument);
}

void* bsearch(const void* element, const void* array, size_t arrayLength, size_t elementSize,
		int (*compare)(const void*, const void*)) {
	int result = searchUtilsBinarySearchComparingElements(element, array, arrayLength, elementSize, compare);
	if (result < 0) {
		return NULL;
	} else {
		return (void*) (array + (result * elementSize));
	}
}

/*
 * References:
 * - https://en.wikipedia.org/wiki/Linear_congruential_generator
 */
static uint32_t randSeed = 1;
int rand(void) {
	randSeed = (1664525U * randSeed + 1013904223U) % (((uint32_t) RAND_MAX) + 1);
	assert(0 <= randSeed && randSeed <= RAND_MAX);
	return (int) randSeed;
}

void srand(unsigned int seed) {
	randSeed = seed;
}

static uint32_t randomSeed = 1;
long int random(void) {
	randSeed = (1664525U * randSeed + 1013904223U) % (((uint32_t) RAND_MAX) + 1);
	assert(0 <= randSeed && randSeed <= RAND_MAX);
	return (int) randSeed;
}

void srandom(unsigned int seed) {
	randomSeed = seed;
}

int setuid(uid_t uid) {
	// TODO: Implement me!
	return 0;
}

uid_t getuid(void) {
	// TODO: Implement me!
	return 0;
}

uid_t geteuid(void) {
	// TODO: Implement me!
	return 0;
}

int setgid(gid_t groupdId) {
	// TODO: Implement me!
	return 0;
}

gid_t getgid(void) {
	// TODO: Implement me!
	return 0;
}

gid_t getegid(void) {
	// TODO: Implement me!
	return 0;
}

int system(const char* command) {
	/*
	 * References:
	 * - https://pubs.opengroup.org/onlinepubs/9699919799/functions/system.html#
	 * - https://man7.org/linux/man-pages/man3/system.3.html
	 */

	struct sigaction oldActionForSigInt;
	struct sigaction oldActionForSigQuit;

	struct sigaction newAction;
	memset(&newAction, 0, sizeof(struct sigaction));
	newAction.sa_handler = SIG_IGN;

	sigset_t oldSet;
	sigset_t newSet;
	sigemptyset(&newSet);
	sigaddset(&newSet, SIGCHLD);

	sigaction(SIGINT, &newAction, &oldActionForSigInt);
	sigaction(SIGQUIT, &newAction, &oldActionForSigQuit);
	sigprocmask(SIG_BLOCK, &newSet, &oldSet);

	int result = -1;
	pid_t childProcessId = fork();
	if (childProcessId == 0) {
		sigaction(SIGINT, &oldActionForSigInt, NULL);
		sigaction(SIGQUIT, &oldActionForSigQuit, NULL);
		sigprocmask(SIG_SETMASK, &oldSet, NULL);

		execl("/bin/sh", "sh", "-c", command, NULL);

		_exit(127);

	} else if (childProcessId > 0) {
		int status;
		while (true) {
			if (waitpid(childProcessId, &status, 0) == -1) {
				if (errno == EINTR) {
					break;
				}
			} else {
				result = status;
				break;
			}
		}
	}

	sigaction(SIGINT, &oldActionForSigInt, NULL);
	sigaction(SIGQUIT, &oldActionForSigQuit, NULL);
	sigprocmask(SIG_SETMASK, &oldSet, NULL);

	return result;
}

static int addToEnvironmentVariables(const char* keyValuePairToBeAdded, const char* key, const char* value, bool overwrite) {
	int errorId = 0;

	if (keyValuePairToBeAdded != NULL) {
		assert(key == NULL);
		assert(value == NULL);

		const char* keyEnd = strchr(keyValuePairToBeAdded, '=');
		if (keyEnd != NULL && keyEnd != keyValuePairToBeAdded) {
			key = strndup(keyValuePairToBeAdded, keyEnd - keyValuePairToBeAdded);
			if (key != NULL) {
				value = keyEnd + 1;
			} else {
				errorId = EINVAL;
			}

		} else {
			key = keyValuePairToBeAdded;
		}

	} else {
		if (key == NULL || strlen(key) == 0 || strchr(key, '=') != NULL) {
			errorId = EINVAL;
		}
	}

	if (errorId == 0) {
		struct DynamicArray dynamicArray;
		dynamicArrayInitialize(&dynamicArray, sizeof(char*), NULL, &memoryAllocatorRelease, &memoryAllocatorResize);

		bool foundKey = false;
		bool replaceEnviron = true;
		const char* keyValuePair;
		for (int i = 0; (keyValuePair = environ[i]) != NULL; i++) {
			if (!foundKey) {
				char* separator = strchr(keyValuePair, '=');
				assert(separator != NULL);
				size_t keyLength = separator - keyValuePair;
				if (strncmp(keyValuePair, key, keyLength) == 0) {
					foundKey = true;
					if (overwrite) {
						if (value != NULL) {
							char* newKeyValuePair;

							if (keyValuePairToBeAdded != NULL) {
								newKeyValuePair = (char*) keyValuePairToBeAdded;

							} else {
								newKeyValuePair = malloc(sizeof(char) * (strlen(key) + 1 + strlen(value) + 1));
								if (newKeyValuePair == NULL) {
									errorId = ENOMEM;
									break;
								}
								sprintf(newKeyValuePair, "%s=%s", key, value);
							}
							if (dynamicArrayInsertAfterLast(&dynamicArray, &newKeyValuePair) == NULL) {
								errorId = ENOMEM;
								break;
							}
						}

					} else {
						replaceEnviron = false;
						break;
					}

				} else {
					if (dynamicArrayInsertAfterLast(&dynamicArray, &keyValuePair) == NULL) {
						errorId = ENOMEM;
						break;
					}
				}

			} else {
				if (dynamicArrayInsertAfterLast(&dynamicArray, &keyValuePair) == NULL) {
					errorId = ENOMEM;
					break;
				}
			}
		}

		if (errorId == 0 && replaceEnviron) {
			if (!foundKey && value != NULL) {
				char* newKeyValuePair;
				if (keyValuePairToBeAdded != NULL) {
					newKeyValuePair = (char*) keyValuePairToBeAdded;

				} else {
					newKeyValuePair = malloc(sizeof(char) * (strlen(key) + 1 + strlen(value) + 1));
					if (newKeyValuePair != NULL) {
						sprintf(newKeyValuePair, "%s=%s", key, value);
					} else {
						errorId = ENOMEM;
					}
				}

				if (dynamicArrayInsertAfterLast(&dynamicArray, &newKeyValuePair) == NULL) {
					errorId = ENOMEM;
				}
			}

			if (errorId == 0) {
				char* nullKeyValuePair = NULL;
				if (dynamicArrayInsertAfterLast(&dynamicArray, &nullKeyValuePair) == NULL) {
					errorId = ENOMEM;
				} else {
					free(lastAllocatedEnviron);
					lastAllocatedEnviron = environ = dynamicArrayGetArray(&dynamicArray);
				}
			}
		}

		if (errorId != 0) {
			dynamicArrayClear(&dynamicArray, true);
		}
	}

	if (errorId != 0) {
		errno = errorId;
		return -1;

	} else {
		return 0;
	}
}

int putenv(char* keyValuePairToBeAdded) {
	return addToEnvironmentVariables(keyValuePairToBeAdded, NULL, NULL, true);
}

char* getenv(const char* key) {
	const char* keyValuePair;
	for (int i = 0; (keyValuePair = environ[i]) != NULL; i++) {
		char* separator = strchr(keyValuePair, '=');
		size_t keyLength = separator - keyValuePair;
		if (strncmp(keyValuePair, key, keyLength) == 0) {
			return separator + 1;
		}
	}

	return NULL;
}

int setenv(const char* key, const char* value, int overwrite) {
	return addToEnvironmentVariables(NULL, key, value, overwrite);
}

int clearenv(void) {
	free(lastAllocatedEnviron);
	environ = (void*) defaultEnviron;
	return 0;
}

int unsetenv(const char* key) {
	return addToEnvironmentVariables(NULL, key, NULL, true);
}

static int nextTemporaryFileId = 0;
int mkstemp(char* template) {
	int fileDescriptorIndex = -1;
	int errorId = 0;

	if (template != NULL) {
		size_t length = strlen(template);
		if (length >= 6 && strcmp("XXXXXX", template + (length - 6)) == 0) {
			while (nextTemporaryFileId <= 0xFFFFFF) {
				sprintf(template + (length - 6), "%06X", nextTemporaryFileId++);
				fileDescriptorIndex = open(template, O_RDWR | O_CREAT | O_EXCL);
				if (fileDescriptorIndex >= 0) {
					return fileDescriptorIndex;
				}
			}

			errorId = EEXIST;
			nextTemporaryFileId = 0;

		} else {
			errorId = EINVAL;
		}

	} else {
		errorId = EINVAL;
	}

	errno = errorId;
	return fileDescriptorIndex;
}

char* mktemp(char* template){
	assert(false);
	return NULL;
}

int mkstemps(char* template, int suffixlen) {
	assert(false);
	return -1;
}

char* mkdtemp(char* template) {
	assert(false);
	return template;
}

long double strtold(const char* string, char** startOfNotProcessedString) {
	assert(false);
	return 0;
}

double strtod(const char* string, char** startOfNotProcessedString) {
	long double result;
	errno = scannerParseFloat(string, true, (const char**) startOfNotProcessedString, &result);
	if (!isnan(result) && !isinf(result) && errno == 0 && !(DBL_MIN <= result && result <= DBL_MAX)) {
		errno = ERANGE;
		if (result > 0) {
			result = DBL_MAX;
		} else {
			result = DBL_MIN;
		}
	}

	return (double) result;
}

double atof(const char* string) {
	return strtod(string, NULL);
}

size_t mbstowcs(wchar_t* dest, const char* src, size_t n) {
	// TODO: Implement me!
	assert(false);
	return 0;
}
