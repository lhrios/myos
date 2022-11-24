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

#ifndef MATH_UTILS_H
	#define MATH_UTILS_H

	#include <limits.h>
	#include <stdbool.h>
	#include <stdint.h>

	#define mathUtilsMax(a,b) \
		({ __typeof__ (a) _a = (a); \
			__typeof__ (b) _b = (b); \
			_a > _b ? _a : _b; })

	#define mathUtilsMin(a,b) \
		({ __typeof__ (a) _a = (a); \
			__typeof__ (b) _b = (b); \
			_a < _b ? _a : _b; })

	inline __attribute__((always_inline)) uint32_t mathUtilsCeilOfUint32Division(uint32_t dividend, uint32_t divisor) {
		uint32_t result = dividend / divisor;
		if (dividend % divisor != 0) {
			result++;
		}
		return result;
	}

	inline __attribute__((always_inline)) int32_t mathUtilsClampInt32(int32_t value, int32_t min, int32_t max) {
		value = mathUtilsMax(min, value);
		value = mathUtilsMin(max, value);
		return value;
	}

	inline __attribute__((always_inline)) uint32_t mathUtilsLog2ForPowerOf2(uint32_t x) {
		uint32_t y;
		__asm__ __volatile__(
			"bsr %1, %0"
			: "=r"(y)
			: "m"(x)
			: "cc");
		return y;
	}

	inline __attribute__((always_inline)) bool mathUtilsIsPowerOf(uint32_t value, uint32_t base) {
		if (value == 0) {
			return false;

		} else if (value == 1) {
			return true;

		} else {
			while (value % base == 0) {
				value = value / base;
			}

			return value == 1;
		}
	}

	long double mathUtilsTrunc(long double value);
	long double mathUtilsLogE(long double value, int* errorId);
	long double mathUtilsLog10(long double value, int* errorId);
	long double mathUtilsPow(long double x, long double y, int* errorId);
	long double mathUtilsFmod(long double x, long double y, int* errorId);
	long double mathUtilsFloor(long double value);
	long double mathUtilsCeil(long double value);

	static inline __attribute__((always_inline)) long double mathUtilsFabsl(long double value) {
		double result;

		__asm__ __volatile__(
			"fldt %1;"
			"fabs;"
			"fstpt %0;"
			: "=m" (result)
			: "m" (value)
			: "memory"
		);

		 return result;
	}

	static inline __attribute__((always_inline)) long double mathUtilsApproximatelyEquals(long double value1, long double value2, long double delta) {
		return mathUtilsFabsl(value1 - value2) <= delta;
	}

#endif
