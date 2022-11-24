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

#ifndef GLOB_H
	#define GLOB_H

	#include <stddef.h>

	#include "user/util/dynamic_array.h"

	typedef struct {
		size_t gl_pathc;
		char** gl_pathv;
		size_t gl_offs;

		int slotsReservedAtTheBeginning;
		struct DynamicArray result;
	} glob_t;

	int glob(const char* pattern, int flags, int (*errorCallBack)(const char*, int), glob_t* globInstance);
	void globfree(glob_t* globInstance);

	#define GLOB_ERR (1 << 0)
   #define GLOB_MARK (1 << 1)
   #define GLOB_NOSORT (1 << 2)
   #define GLOB_DOOFFS (1 << 3)
   #define GLOB_NOCHECK (1 << 4)
   #define GLOB_APPEND (1 << 5)
   #define GLOB_NOESCAPE (1 << 6)

	// TODO: Implement me!
	#define GLOB_PERIOD (1 << 7)
   #define GLOB_ALTDIRFUNC (1 << 8)
   #define GLOB_BRACE (1 << 9)
   #define GLOB_NOMAGIC (1 << 10)
   #define GLOB_TILDE (1 << 11)
   #define GLOB_TILDE_CHECK (1 << 12)
   #define GLOB_ONLYDIR (1 << 13)

   #define GLOB_NOSPACE 1
   #define GLOB_ABORTED 2
   #define GLOB_NOMATCH 3

#endif
