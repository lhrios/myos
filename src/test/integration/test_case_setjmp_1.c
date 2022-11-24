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

#include "test/integration_test.h"

#define VALUE 1234

static jmp_buf longJumpContext;

static void procedure5(void) {
	longjmp(longJumpContext, 0);
	assert(false);
}

static void procedure4(void) {
	longjmp(longJumpContext, VALUE);
	assert(false);
}

static void procedure3(void) {
	procedure4();
	assert(false);
}

static void procedure2(void) {
	procedure3();
	assert(false);
}

static void procedure1(void) {
	procedure2();
	assert(false);
}

static int integerComparator(const int* a, const int* b) {
	return *a - *b;
}

int main(int argc, char** argv) {
	int result;
	result = setjmp(longJumpContext);
	if (result != 0) {
		assert(result == VALUE);

		/* Do something "complex". */
		int* array = malloc(sizeof(int) * 5);
		array[0] = 1;
		array[1] = -1;
		array[2] = 2;
		array[3] = 0;
		array[4] = -2;

		result = setjmp(longJumpContext);
		if (result == 0) {
			procedure5();
			assert(false);

		} else {
			assert(result == 1);
		}

		result = setjmp(longJumpContext);
		if (result == 0) {
			longjmp(longJumpContext, 0);
			assert(false);

		} else {
			assert(result == 1);
		}

		qsort(array, 5, sizeof(int), (int (*)(const void*, const void*)) &integerComparator);

		assert(array[0] == -2);
		assert(array[1] == -1);
		assert(array[2] == 0);
		assert(array[3] == 1);
		assert(array[4] == 2);

	} else {
		procedure1();
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
