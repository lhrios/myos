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
#include <math.h>
#include <stdlib.h>

#include "util/math_utils.h"

// TODO: The following implementation is overflowing on cases where default libc implementation does not.
long double mathUtilsPow(long double x, long double y, int* errorId) {
	if (x < 0 && mathUtilsFmod(y, 1, errorId) != 0) {
		if (errorId != NULL) {
			*errorId = EDOM;
		}
		return -NAN;

	} else if (x == 0 && y < 0) {
		if (errorId != NULL) {
			*errorId = ERANGE;
		}
		return INFINITY;
	}

	long double result;
	__asm__ __volatile__(
		"fldt %2;"
		"fldt %1;"
		"fabs;"
		"fyl2x;"

		"fld %%st(0);"

		"fld1;"
		"fxch;"
		"fprem;"

		"f2xm1;"
		"faddp;"

		"fscale;"

		"ffree %%st(1);"

		"fstpt %0;"

		: "=m"(result)
		: "m"(x), "m"(y)
	   : "memory"
	);

	if (x < 0 && mathUtilsFmod(y, 2, errorId) != 0) {
		result *= -1;
	}

	return result;
}

enum LogarithmBase {
	LOGARITHM_BASE_10,
	LOGARITHM_BASE_E
};

long double mathUtilsLog(long double value, enum LogarithmBase logarithmBase, int* errorId) {
	if (value == 0) {
		if (errorId != NULL) {
			*errorId = ERANGE;
		}
		return -INFINITY;

	} else if (value < 0) {
		if (errorId != NULL) {
			*errorId = EDOM;
		}
		return -NAN;
	}

	long double result;
	__asm__ __volatile__(
		"cmp $0, %%eax;"
		"je pushLog2_10;"
		"fldl2e;"
		"jmp afterPushLog2_10;"
		"pushLog2_10: fldl2t;"
		"afterPushLog2_10: fld1;"
		"fldt %1;"
		"fsub %%ST(1);"
		"fyl2xp1;"
		"fdivp;"
		"fstpt %0;"
		: "=m" (result)
		: "m" (value), "a"(logarithmBase)
		: "memory"
	);
	return result;
}

long double mathUtilsLog10(long double value, int* errorId) {
	return mathUtilsLog(value, LOGARITHM_BASE_10, errorId);
}

long double mathUtilsLogE(long double value, int* errorId) {
	return mathUtilsLog(value, LOGARITHM_BASE_E, errorId);
}

long double mathUtilsFmod(long double x, long double y, int* errorId) {
	if (isinf(x) || y == 0.) {
		if (errorId != NULL) {
			*errorId = EDOM;
		}
	}

	long double result;
	__asm__ __volatile__(
		"fldt %2;"
		"fldt %1;"
		"fprem;"
		"ffree %%st(1);"
		"fstpt %0;"
		: "=m"(result)
		: "m"(x), "m"(y)
	   : "memory"
	);

	return result;
}

long double mathUtilsFloor(long double value) {
	long double remainder = mathUtilsFmod(value, 1, NULL);
	if (remainder == 0) {
		return value;

	} else if (remainder < 0) {
		return value - (1 + remainder);

	} else {
		return value - remainder;
	}
}

long double mathUtilsCeil(long double value) {
	long double remainder = mathUtilsFmod(value, 1, NULL);
	if (remainder == 0) {
		return value;

	} else if (remainder < 0) {
		return value + (1 + remainder);

	} else {
		return value + (1 - remainder);
	}
}

long double mathUtilsTrunc(long double value) {
	if (value > 0) {
		return mathUtilsFloor(value);
	} else {
		return mathUtilsCeil(value);
	}
}
