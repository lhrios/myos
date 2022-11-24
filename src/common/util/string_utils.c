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

#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "util/string_utils.h"

char* stringUtilsLeftTrim(char* string) {
	char* adjustedString = string;
	while (isspace(*adjustedString)) {
		adjustedString++;
	}

	strcpy(string, adjustedString);

	return string;
}

char* stringUtilsRightTrim(char* string) {
	int length = strlen(string);
	if (length == 0) {
		return string;

	} else {
		char* stringEnd = string + length - 1;
		while (isspace(*stringEnd)) {
			stringEnd--;
		}

		stringEnd[1] = '\0';

		return string;
	}
}

char* stringUtilsTrim(char* string) {
	return stringUtilsRightTrim(stringUtilsLeftTrim(string));
}

bool stringUtilsIsDigitOnly(const char* string) {
	return stringUtilsNIsDigitOnly(string, SIZE_MAX);
}

bool stringUtilsStartsWith(const char* string, const char* prefix) {
	if (string == NULL || prefix == NULL) {
		return false;
	} else {
		return strncmp(prefix, string, strlen(prefix)) == 0;
	}
}

bool stringUtilsNIsDigitOnly(const char* string, size_t count) {
	if (*string == '\0' || count == 0) {
		return false;

	} else {
		while (*string && count > 0) {
			if (!isdigit(*string)) {
				return false;
			}
			string++;
			count--;
		}

		return true;
	}
}

char* stringUtilsParseTokens(char* string, const char* delimiters, char** context) {
	if (string == NULL) {
		string = *context;
	}

	while (*string != '\0' && strchr(delimiters, *string) != NULL) {
		string++;
	}

	if (*string == '\0') {
		return NULL;

	} else {
		char* token = string;
		string++;

		while (*string != '\0' && strchr(delimiters, *string) == NULL) {
			string++;
		}
		if (*string == '\0') {
			*context = string;
		} else {
			*string = '\0';
			*context = string + 1;
		}

		return token;
	}
}

char* stringUtilsNToUpperCase(char* string, size_t count) {
	for (size_t i = 0; i < count && string[i] != '\0'; i++) {
		string[i] = toupper(string[i]);
	}

	return string;
}

char* stringUtilsToUpperCase(char* string) {
	return stringUtilsNToUpperCase(string, SIZE_MAX);
}
