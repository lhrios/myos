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
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "test/integration_test.h"

#define VALUE -1234

static sigjmp_buf longJumpContext;

static volatile bool done = false;

static void handleSignal(int signalId) {
	switch (signalId) {
		case SIGUSR1: {
			raise(SIGUSR2);
		} break;

		case SIGUSR2: {
			raise(SIGWINCH);
		} break;

		case SIGWINCH: {
			done = true;
		} break;

		default:
			assert(false);
			break;
	}
}

static int integerComparator(const int* a, const int* b) {
	return *b - *a;
}

static void procedure1(void) {
	sigset_t signalsSet;
	sigemptyset(&signalsSet);
	sigaddset(&signalsSet, SIGUSR1);
	sigaddset(&signalsSet, SIGUSR2);
	sigaddset(&signalsSet, SIGWINCH);
	sigaddset(&signalsSet, SIGTSTP);
	int result = sigprocmask(SIG_SETMASK, &signalsSet, NULL);
	assert(result == 0);

	siglongjmp(longJumpContext, 0);
	assert(false);
}

int main(int argc, char** argv) {
	int result;

	sigset_t signalsSet;
	sigemptyset(&signalsSet);
	sigaddset(&signalsSet, SIGUSR1);
	sigaddset(&signalsSet, SIGUSR2);
	sigaddset(&signalsSet, SIGWINCH);

	struct sigaction act;

	memcpy(&act.sa_mask, &signalsSet, sizeof(sigset_t));
	act.sa_flags = 0;
	act.sa_handler = &handleSignal;
	act.sa_sigaction = NULL;
	result = sigaction(SIGUSR1, &act, NULL);
	assert(result == 0);

	memcpy(&act.sa_mask, &signalsSet, sizeof(sigset_t));
	act.sa_flags = 0;
	act.sa_handler = &handleSignal;
	act.sa_sigaction = NULL;
	result = sigaction(SIGUSR2, &act, NULL);
	assert(result == 0);

	memcpy(&act.sa_mask, &signalsSet, sizeof(sigset_t));
	act.sa_flags = 0;
	act.sa_handler = &handleSignal;
	act.sa_sigaction = NULL;
	result = sigaction(SIGWINCH, &act, NULL);
	assert(result == 0);

	result = sigsetjmp(longJumpContext, true);
	if (result == 0) {
		result = sigprocmask(SIG_SETMASK, &signalsSet, NULL);
		assert(result == 0);

		result = raise(SIGUSR1);
		assert(result == 0);
		assert(!done);

		siglongjmp(longJumpContext, VALUE);
		assert(false);

	} else {
		assert(result == VALUE);

		result = sigprocmask(SIG_SETMASK, NULL, &signalsSet);
		assert(result == 0);

		assert(!sigismember(&signalsSet, SIGUSR1));
		assert(!sigismember(&signalsSet, SIGUSR2));
		assert(!sigismember(&signalsSet, SIGWINCH));

		while (!done);

		/* Do something "complex". */
		int* array = malloc(10 * sizeof(int));
		array[0] = 1;
		array[1] = -1;
		array[2] = 2;
		array[3] = 0;
		array[4] = -2;
		array[5] = -3;
		array[6] = 3;
		array[7] = 4;
		array[8] = -4;
		array[9] = 5;

		result = sigsetjmp(longJumpContext, 0);
		if (result == 0) {
			procedure1();
			assert(false);

		} else {
			assert(result == 1);
			result = sigprocmask(SIG_SETMASK, NULL, &signalsSet);
			assert(result == 0);
			assert(sigismember(&signalsSet, SIGUSR1));
			assert(sigismember(&signalsSet, SIGUSR2));
			assert(sigismember(&signalsSet, SIGWINCH));
			assert(sigismember(&signalsSet, SIGTSTP));
		}

		qsort(array, 10, sizeof(int), (int (*)(const void*, const void*)) &integerComparator);

		assert(array[0] == 5);
		assert(array[1] == 4);
		assert(array[2] == 3);
		assert(array[3] == 2);
		assert(array[4] == 1);
		assert(array[5] == 0);
		assert(array[6] == -1);
		assert(array[7] == -2);
		assert(array[8] == -3);
		assert(array[9] == -4);

		int element = 0;
		int* result = bsearch(&element, array, 10, sizeof(int), (int (*)(const void*, const void*)) &integerComparator);
		assert(result != NULL);
		assert(*result == 0);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
