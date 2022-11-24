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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

#include "util/math_utils.h"

#define DELTA 0.00001

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	double value1Before = 0.5;
	double value2Before = -1.5;
	double value3Before = 2.5;

	__asm__ __volatile__(
		"fldl %0;"
		"fldl %1;"
		"fldl %2;"
		:
		: "m" (value1Before), "m" (value2Before), "m" (value3Before)
		: "memory"
	);

	sleep(30);

	double value1After;
	double value2After;
	double value3After;

	__asm__ __volatile__(
		"fstpl %2;"
		"fstpl %1;"
		"fstpl %0;"
		:
		: "m" (value1After), "m" (value2After), "m" (value3After)
		: "memory"
	);

	assert(mathUtilsApproximatelyEquals(value1Before, value1After, DELTA));
	assert(mathUtilsApproximatelyEquals(value2Before, value2After, DELTA));
	assert(mathUtilsApproximatelyEquals(value3Before, value3After, DELTA));

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
