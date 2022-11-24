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

#ifndef STRING_UTILS_H
	#define STRING_UTILS_H

	#include <stdbool.h>
	#include <stdlib.h>

	char* stringUtilsLeftTrim(char* string);
	char* stringUtilsRightTrim(char* string);
	char* stringUtilsTrim(char* string);

	bool stringUtilsIsDigitOnly(const char* string);
	bool stringUtilsNIsDigitOnly(const char* string, size_t count);

	bool stringUtilsStartsWith(const char* string, const char* prefix);

	char* stringUtilsParseTokens(char* string, const char* delimiters, char** context);

	char* stringUtilsNToUpperCase(char* string, size_t count);
	char* stringUtilsToUpperCase(char* string);

#endif
