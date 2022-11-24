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

#ifndef LIMITS_H
	#define LIMITS_H

	#include <stdint.h>

	#include "kernel/limits.h"
	#include "kernel/memory_manager_constants.h"

	#define MB_LEN_MAX 1
	#define CHAR_BIT 8
	#define CHAR_MIN -128
   #define CHAR_MAX 127
	#define SCHAR_MIN CHAR_MIN
	#define SCHAR_MAX CHAR_MAX

	#define UCHAR_MAX 255

	#define SHRT_MIN -32768
	#define SHRT_MAX +32767

	#define UINT16_MAX 65535
	#define USHRT_MAX 65535

	#define LONG_MIN -2147483648
	#define LONG_MAX 2147483647

	#define ULONG_MAX 4294967295U

	#define LLONG_MIN -9223372036854775808L
	#define LLONG_MAX +9223372036854775807L

	#define ULLONG_MAX 18446744073709551615UL

	#define INT_MIN -2147483648
	#define INT_MAX 2147483647

	#define UINT_MAX 4294967295U

	#define ARG_MAX 4096U

	#define PATH_MAX PATH_MAX_LENGTH

	#define PIPE_BUF PAGE_FRAME_SIZE

	#define ATEXIT_MAX 32

	#define OPEN_MAX MAX_FILE_DESCRIPTORS_PER_PROCESS

#endif
