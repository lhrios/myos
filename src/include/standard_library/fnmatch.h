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

#ifndef FNMATCH_H
	#define FNMATCH_H

	#define FNM_NOMATCH 1

	#define FNM_PERIOD (1 << 0)
	#define FNM_PATHNAME (1 << 1)
	#define FNM_FILE_NAME FNM_PATHNAME
	#define FNM_LEADING_DIR (1 << 2)
	#define FNM_NOESCAPE (1 << 3)
	#define FNM_CASEFOLD (1 << 4)

	int fnmatch(const char* pattern, const char* string, int flags);

#endif
