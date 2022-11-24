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

#ifndef STDINT_H
	#define STDINT_H

	typedef char int8_t;
	typedef int8_t int_least8_t;
	typedef unsigned char uint8_t;
	typedef uint8_t uint_least8_t;
	_Static_assert(sizeof(int8_t) == 1, "Expecting int8_t with 1 byte.");
	_Static_assert(sizeof(uint8_t) == 1, "Expecting int8_t with 1 byte.");
	#define UINT8_MAX 255

	typedef short int int16_t;
	typedef int16_t int_least16_t;
	typedef unsigned short int uint16_t;
	typedef uint16_t uint_least16_t;
	_Static_assert(sizeof(int16_t) == 2, "Expecting int8_t with 2 bytes.");
	_Static_assert(sizeof(uint16_t) == 2, "Expecting uint16_t with 2 bytes.");

	typedef int int32_t;
	typedef unsigned int uint32_t;
	_Static_assert(sizeof(int32_t) == 4, "Expecting int32_t with 4 bytes.");
	_Static_assert(sizeof(uint32_t) == 4, "Expecting uint32_t with 4 bytes.");
	#define UINT32_MAX 4294967295U

	typedef long long int int64_t;
	typedef unsigned long long int uint64_t;
	typedef uint64_t uint_fast64_t;
	_Static_assert(sizeof(int64_t) == 8, "Expecting int64_t with 8 bytes.");
	_Static_assert(sizeof(uint64_t) == 8, "Expecting uint64_t with 8 bytes.");
	#define UINT_FAST64_MAX 18446744073709551615UL

	typedef int32_t intmax_t;
	typedef uint32_t uintmax_t;
	#define UINTMAX_MAX UINT32_MAX
	#define INTMAX_MIN -2147483648
	#define INTMAX_MAX 2147483647

	typedef int32_t intptr_t;
	typedef uint32_t uintptr_t;

	#define SIZE_MAX 4294967295U

	#define PTRDIFF_MAX 2147483647
#endif
