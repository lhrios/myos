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

#ifndef INTTYPES_H
	#define INTTYPES_H

	#include <stdint.h>
	#include <stdlib.h>

	#define PRIuMAX "u"
	#define PRId64 "lld"
	#define PRIu64 "llu"
	#define PRIx64 "llx"
	#define PRIdMAX "ld"

	#define strtoumax(string, startOfNotProcessedString, base) ((uintmax_t) strtoul(string, startOfNotProcessedString, base))
	#define strtoimax(string, startOfNotProcessedString, base) ((intmax_t) strtol(string, startOfNotProcessedString, base))

#endif
