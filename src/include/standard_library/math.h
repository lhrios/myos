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

#ifndef MATH_H
	#define MATH_H

	#include <errno.h>

	#include "util/math_utils.h"

	#define NAN __builtin_nanf("")
	#define INFINITY __builtin_inff()
	#define HUGE_VAL __builtin_inff()

	#define M_PI 3.14159265358979323846
	#define M_PI_2 (M_PI / 2)
	#define M_PI_4 (M_PI / 4)

	#define M_E 2.7182818284590452354

	#define isnan(value) __builtin_isnan(value)
	#define isinf(value) __builtin_isinf(value)

	#define fabs(value) ((double) fabsl((long double) value))
	#define fabsl(value) mathUtilsFabsl(value)

	double sqrt(double value);
	double pow(double x, double y);
	#define powl(x, y) mathUtilsPow((long double) x, (long double) y, &errno)
	#define exp(value) pow(M_E, value)

	#define trunc(value) mathUtilsTrunc((long double) value)
	#define floor(value) mathUtilsFloor((long double) value)
	#define ceil(value) mathUtilsCeil((long double) value)

	double cos(double value);
	double sin(double value);
	#define atan(value) atan2(value, 1)
	double atan2(double x, double y);

	#define log(value) ((double) mathUtilsLogE((long double) value, &errno))
	double log2(double value);
	#define log10(value) ((double) mathUtilsLog10((long double) value, &errno))

	double fmod(double x, double y);

	#define signbit(value) __builtin_signbit(value)

#endif
