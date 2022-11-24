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

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util/math_utils.h"

const char * const sys_siglist[] = {
	"",
	"SIGUSR1",
	"SIGUSR2",
	"SIGCHLD",
	"SIGALRM",
	"SIGVTALRM",
	"SIGPROF",
	"SIGWINCH",
	"SIGURG",
	"SIGHUP",
	"SIGTTOU",
	"SIGTTIN",
	"SIGTERM",
	"SIGQUIT",
	"SIGABRT",
	"SIGPIPE",
	"SIGXFSZ",
	"SIGXCPU",
	"SIGSYS",
	"SIGTRAP",
	"SIGINT",
	"SIGFPE",
	"SIGBUS",
	"SIGILL",
	"SIGSEGV",
	"SIGCONT",
	"SIGTSTP",
	"SIGSTOP",
	"SIGKILL"
};
_Static_assert(sizeof(sys_siglist) / sizeof(char*) - 1 == NUMBER_OF_SIGNALS, "Expecting a different number of signals.");
const int sys_nsig = NUMBER_OF_SIGNALS;

char* strdup(const char* string) {
	return strndup(string, UINT_MAX);
}

char* strndup(const char* string, size_t size) {
	size_t stringLength = strlen(string);
	size_t newStringLength = mathUtilsMin(size, stringLength);
	char* newString = malloc(sizeof(char) * (newStringLength + 1));
	if (newString != NULL) {
		memcpy(newString, string, sizeof(char) * newStringLength);
		newString[newStringLength] = '\0';
		return newString;

	} else {
		return NULL;
	}
}

#define ERROR_DESCRIPTION_MAX_LENGTH 128
static char errorDescription[ERROR_DESCRIPTION_MAX_LENGTH];
char* strerror(int errnum) {
	if (0 < errnum && errnum < sys_nerr && strlen(sys_errlist[errnum]) > 0) {
		strlcpy(errorDescription, sys_errlist[errnum], ERROR_DESCRIPTION_MAX_LENGTH);
	} else {
		snprintf(errorDescription, ERROR_DESCRIPTION_MAX_LENGTH, "Unknown error %d", errnum);
	}
	return errorDescription;
}
