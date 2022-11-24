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

#ifndef FLOAT_H
	#define FLOAT_H

	#define FLT_MIN 1.175494351E-38
	#define FLT_MAX 3.402823466E+38

	#define DBL_MIN 2.2250738585072014E-308
	#define DBL_MAX 1.7976931348623158E+308

	#define DBL_MIN_EXP -1021
	#define DBL_MAX_EXP 1024

	#define LDBL_MIN_EXP -16381
	#define LDBL_MAX_EXP 16384

	#define LDBL_MIN 3.36210314311209350626267781732175260E-4932L
   #define LDBL_MAX 1.18973149535723176502126385303097021E+4932L

	#define DBL_MANT_DIG 53
	#define LDBL_MANT_DIG 64

	#define DBL_MAX_10_EXP 308

#endif
