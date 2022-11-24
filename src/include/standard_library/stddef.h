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

#ifndef STDDEF_H
	#define STDDEF_H

	#define NULL ((void*) 0)

	typedef unsigned int size_t;
	_Static_assert(sizeof(size_t) == 4, "Expecting size_t with 4 bytes.");

	typedef int ptrdiff_t;
	_Static_assert(sizeof(ptrdiff_t) == 4, "Expecting ptrdiff_t with 4 bytes.");

	#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)

	typedef struct {
		char __max_align_long_double[16];
	} max_align_t;
	_Static_assert(sizeof(max_align_t) >= sizeof(long long), "Expecting sizeof(max_align_t) greater than or equals to sizeof(long long int).");
	_Static_assert(sizeof(max_align_t) >= sizeof(long double), "Expecting sizeof(max_align_t) greater than or equals to sizeof(long double).");

	typedef char wchar_t;

#endif
