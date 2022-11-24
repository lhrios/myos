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

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

#include "util/math_utils.h"

#define DELTA 0.00001

static void testSignbit(void) {
	assert(signbit(NAN) == 0);
	assert(signbit(-NAN) != 0);

	assert(signbit(INFINITY) == 0);
	assert(signbit(-INFINITY) != 0);

	assert(signbit(5.) == 0);
	assert(signbit(-5.) != 0);

	assert(signbit(0.) == 0);
	assert(signbit(-0.) != 0);

	assert(signbit(M_PI) == 0);
	assert(signbit(-M_PI) != 0);

	assert(signbit(M_E) == 0);
	assert(signbit(-M_E) != 0);
}

static void testIsnan(void) {
	assert(isnan(0./0.));
	assert(isnan(NAN));
	assert(!isnan(5.));
	assert(!isnan(-5.));
	assert(!isnan(0.));
	assert(!isnan(-0.));
}

static void testIsinf(void) {
	assert(isinf(5./0.));
	assert(isinf(-5./0.));
	assert(isinf(INFINITY));
	assert(isinf(-INFINITY));
	assert(!isinf(NAN));
	assert(!isinf(5.));
	assert(!isinf(-5.));
	assert(!isnan(0.));
	assert(!isnan(-0.));
}

static void testFmod(void) {
	double result;

	result = fmod(100, 3);
	assert(mathUtilsApproximatelyEquals(result, 1, DELTA));

	result = fmod(2, -1.5);
	assert(mathUtilsApproximatelyEquals(result, 0.5, DELTA));

	result = fmod(-2, -1.5);
	assert(mathUtilsApproximatelyEquals(result, -0.5, DELTA));

	result = fmod(-1.5, 1);
	assert(mathUtilsApproximatelyEquals(result, -0.5, DELTA));

	errno = 0;
	result = fmod(-2, 0);
	assert(errno == EDOM);
	assert(isnan(result));

	errno = 0;
	result = fmod(INFINITY, 0);
	assert(errno == EDOM);
	assert(isnan(result));

	errno = 0;
	result = fmod(-INFINITY, 0);
	assert(errno == EDOM);
	assert(isnan(result));
}

static void testPow(void) {
	double result;

	result = pow(-5, 3);
	assert(mathUtilsApproximatelyEquals(result, -125, DELTA));

	result = pow(5, 2);
	assert(mathUtilsApproximatelyEquals(result, 25, DELTA));

	result = pow(-5, 2);
	assert(mathUtilsApproximatelyEquals(result, 25, DELTA));

	result = pow(25, 0.5);
	assert(mathUtilsApproximatelyEquals(result, 5, DELTA));

	result = pow(125, 1./3.);
	assert(mathUtilsApproximatelyEquals(result, 5, DELTA));

	errno = 0;
	result = pow(-5, -1.5);
	assert(errno == EDOM);
	assert(isnan(result));

	errno = 0;
	result = pow(0, -1);
	assert(errno == ERANGE);
	assert(isinf(result));
}

static void testLog(void) {
	double result;

	/* loge: */

	result = log(1000);
	assert(mathUtilsApproximatelyEquals(result, 6.90775, DELTA));

	result = log(1);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	errno = 0;
	result = log(0);
	assert(errno == ERANGE);
	assert(isinf(result));

	errno = 0;
	result = log(-1);
	assert(errno == EDOM);
	assert(isnan(result));

	/* log2: */

	result = log2(1024);
	assert(mathUtilsApproximatelyEquals(result, 10, DELTA));

	result = log2(2);
	assert(mathUtilsApproximatelyEquals(result, 1, DELTA));

	result = log2(1);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	errno = 0;
	result = log2(0);
	assert(errno == ERANGE);
	assert(isinf(result));

	errno = 0;
	result = log2(-1);
	assert(errno == EDOM);
	assert(isnan(result));

	/* log10: */

	result = log10(1000);
	assert(mathUtilsApproximatelyEquals(result, 3, DELTA));

	result = log10(10);
	assert(mathUtilsApproximatelyEquals(result, 1, DELTA));

	result = log10(1);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	errno = 0;
	result = log10(0);
	assert(errno == ERANGE);
	assert(isinf(result));

	errno = 0;
	result = log10(-1);
	assert(errno == EDOM);
	assert(isnan(result));
}

static void testSin(void) {
	double result;

	result = sin(0);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	result = sin(M_PI_2);
	assert(mathUtilsApproximatelyEquals(result, 1, DELTA));

	result = sin(M_PI);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	result = sin(M_PI + M_PI_2);
	assert(mathUtilsApproximatelyEquals(result, -1, DELTA));

	result = sin(2 * M_PI);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	errno = 0;
	result = sin(INFINITY);
	assert(errno == EDOM);
	assert(isnan(result));
}

static void testCos(void) {
	double result;

	result = cos(0);
	assert(mathUtilsApproximatelyEquals(result, 1, DELTA));

	result = cos(M_PI_2);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	result = cos(M_PI);
	assert(mathUtilsApproximatelyEquals(result, -1, DELTA));

	result = cos(M_PI + M_PI_2);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	result = cos(2 * M_PI);
	assert(mathUtilsApproximatelyEquals(result, 1, DELTA));

	errno = 0;
	result = cos(-INFINITY);
	assert(errno == EDOM);
	assert(isnan(result));
}

static void testExp(void) {
	double result;

	result = exp(1);
	assert(mathUtilsApproximatelyEquals(result, M_E, DELTA));

	result = log(exp(1));
	assert(mathUtilsApproximatelyEquals(result, 1, DELTA));
}

static void testFloor(void) {
	double result;

	result = floor(5.3);
	assert(mathUtilsApproximatelyEquals(result, 5, DELTA));

	result = floor(5);
	assert(mathUtilsApproximatelyEquals(result, 5, DELTA));

	result = floor(5.99999999);
	assert(mathUtilsApproximatelyEquals(result, 5, DELTA));

	result = floor(0);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	result = floor(-1.5);
	assert(mathUtilsApproximatelyEquals(result, -2, DELTA));
}

static void testCeil(void) {
	double result;

	result = ceil(5.3);
	assert(mathUtilsApproximatelyEquals(result, 6, DELTA));

	result = ceil(5);
	assert(mathUtilsApproximatelyEquals(result, 5, DELTA));

	result = ceil(5.9999999);
	assert(mathUtilsApproximatelyEquals(result, 6, DELTA));

	result = ceil(0);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	result = ceil(-1.5);
	assert(mathUtilsApproximatelyEquals(result, -1, DELTA));
}

static void testAtan(void) {
	double result;

	result = atan(0);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	result = atan(1);
	assert(mathUtilsApproximatelyEquals(result, 0.785398, DELTA));

	result = atan(5);
	assert(mathUtilsApproximatelyEquals(result, 1.373401, DELTA));

	result = atan(-10);
	assert(mathUtilsApproximatelyEquals(result, -1.471128, DELTA));

	result = atan(INFINITY);
	assert(mathUtilsApproximatelyEquals(result, 1.570796, DELTA));

	result = atan(-INFINITY);
	assert(mathUtilsApproximatelyEquals(result, -1.570796, DELTA));
}

static void testAtan2(void) {
	double result;

	result = atan2(0, 0);
	assert(mathUtilsApproximatelyEquals(result, 0, DELTA));

	result = atan2(1, 1);
	assert(mathUtilsApproximatelyEquals(result, 0.785398, DELTA));

	result = atan2(1, 2);
	assert(mathUtilsApproximatelyEquals(result, 0.463648, DELTA));

	result = atan2(-5, -5);
	assert(mathUtilsApproximatelyEquals(result, -2.356194, DELTA));

	result = atan2(-3, 2);
	assert(mathUtilsApproximatelyEquals(result, -0.982794, DELTA));

	result = atan2(3, -2);
	assert(mathUtilsApproximatelyEquals(result, 2.158799, DELTA));

	result = atan2(INFINITY, INFINITY);
	assert(mathUtilsApproximatelyEquals(result, 0.785398, DELTA));
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	testSignbit();
	testIsinf();
	testIsnan();
	testFmod();
	testPow();
	testLog();
	testSin();
	testCos();
	testExp();
	testFloor();
	testCeil();
	testAtan();
	testAtan2();

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
