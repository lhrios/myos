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

#include "util/math_utils.h"

double log2(double value) {
	if (value == 0) {
		errno = ERANGE;
		return -INFINITY;

	} else if (value < 0) {
		errno = EDOM;
		return -NAN;
	}

	double result;
	__asm__ __volatile__(
		"fld1;"
		"fldl %1;"
		"fsub %%ST(1);"
		"fyl2xp1;"
		"fstpl %0;"
		: "=m" (result)
		: "m" (value)
		: "memory"
	);
	return result;
}

double cos(double value) {
	if (isinf(value)) {
		errno = EDOM;
		return -NAN;
	}

	double result;
	__asm__ __volatile__(
		"fldl %1;"
		"fcos;"
		"fstpl %0;"
		: "=m" (result)
		: "m" (value)
		: "memory"
	);
	return result;
}

double sin(double value) {
	if (isinf(value)) {
		errno = EDOM;
		return -NAN;
	}

	double result;
	__asm__ __volatile__(
		"fldl %1;"
		"fsin;"
		"fstpl %0;"
		: "=m" (result)
		: "m" (value)
		: "memory"
	);

	return result;
}

double atan2(double x, double y) {
	double result;

	__asm__ __volatile__(
		"fldl %2;"
		"fldl %1;"
		"fpatan;"
		"fstpl %0;"
		: "=m" (result)
		: "m" (y), "m" (x)
		: "memory"
	 );

	return result;
}

double sqrt(double value) {
	if (value < 0) {
		errno = EDOM;
		return NAN;
	}

	double result;

	__asm__ __volatile__(
		"fldl %1;"
		"fsqrt;"
		"fstpl %0;"
		: "=m" (result)
		: "m" (value)
		: "memory"
	);

	return result;
}

double fmod(double x, double y) {
	return (double) mathUtilsFmod(x, y, &errno);
}

double pow(double x, double y) {
	return (double) mathUtilsPow(x, y, &errno);
}
