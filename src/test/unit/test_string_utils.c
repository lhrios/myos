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

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "util/string_utils.h"

static void testLeftTrim(void) {
	char buffer[32];

	strcpy(buffer, " \r\n\v\t\f abc");
	assert(strcmp("abc", stringUtilsLeftTrim(buffer)) == 0);

	strcpy(buffer, "abc \t");
	assert(strcmp("abc \t", stringUtilsLeftTrim(buffer)) == 0);

	strcpy(buffer, "abc");
	assert(strcmp("abc", stringUtilsLeftTrim(buffer)) == 0);

	strcpy(buffer, "	 ");
	assert(strcmp("", stringUtilsLeftTrim(buffer)) == 0);

	strcpy(buffer, "");
	assert(strcmp("", stringUtilsLeftTrim(buffer)) == 0);
}

static void testRightTrim(void) {
	char buffer[32];

	strcpy(buffer, "abc \r\n\v\t\f ");
	assert(strcmp("abc", stringUtilsRightTrim(buffer)) == 0);

	strcpy(buffer, " \tabc");
	assert(strcmp(" \tabc", stringUtilsRightTrim(buffer)) == 0);

	strcpy(buffer, "abc");
	assert(strcmp("abc", stringUtilsRightTrim(buffer)) == 0);

	strcpy(buffer, "	 ");
	assert(strcmp("", stringUtilsRightTrim(buffer)) == 0);

	strcpy(buffer, "");
	assert(strcmp("", stringUtilsRightTrim(buffer)) == 0);
}

static void testTrim(void) {
	char buffer[32];

	strcpy(buffer, "		 abc \r\n\v\t\f ");
	assert(strcmp("abc", stringUtilsTrim(buffer)) == 0);

	strcpy(buffer, " \tab c ");
	assert(strcmp("ab c", stringUtilsTrim(buffer)) == 0);

	strcpy(buffer, "abc");
	assert(strcmp("abc", stringUtilsTrim(buffer)) == 0);

	strcpy(buffer, "  \t\n\r  ");
	assert(strcmp("", stringUtilsTrim(buffer)) == 0);

	strcpy(buffer, "");
	assert(strcmp("", stringUtilsTrim(buffer)) == 0);

	strcpy(buffer, " Z\tZ ");
	assert(strcmp("Z\tZ", stringUtilsTrim(buffer)) == 0);
}

static void testIsDigitOnly(void) {
	assert(!stringUtilsIsDigitOnly("Z\tZ"));
	assert(!stringUtilsIsDigitOnly(""));
	assert(!stringUtilsIsDigitOnly("AA"));

	assert(stringUtilsIsDigitOnly("0"));
	assert(stringUtilsIsDigitOnly("0123456789"));
	assert(stringUtilsIsDigitOnly("0123456789123456789"));

	assert(stringUtilsNIsDigitOnly("012XXXXX", 3));
}

static void testParseTokens(void) {
	const char* delimiters;
	char* result;
	char string[256];
	char* context;

	/* Test 1 */
	strcpy(string, ";;;aaa_/:bbb;;ccc;;;;");
	delimiters = ";:-/_";
	result = stringUtilsParseTokens(string, delimiters, &context);
	assert(result != NULL);
	assert(strcmp(result, "aaa") == 0);

	result = stringUtilsParseTokens(NULL, delimiters, &context);
	assert(result != NULL);
	assert(strcmp(result, "bbb") == 0);

	result = stringUtilsParseTokens(NULL, delimiters, &context);
	assert(result != NULL);
	assert(strcmp(result, "ccc") == 0);

	result = stringUtilsParseTokens(NULL, delimiters, &context);
	assert(result == NULL);

	/* Test 2 */
	strcpy(string, "__");
	delimiters = "_";
	result = stringUtilsParseTokens(string, delimiters, &context);
	assert(result == NULL);

	/* Test 3 */
	strcpy(string, "");
	delimiters = "_";
	result = stringUtilsParseTokens(string, delimiters, &context);
	assert(result == NULL);

	/* Test 4 */
	strcpy(string, "");
	delimiters = "_";
	result = stringUtilsParseTokens(string, delimiters, &context);
	assert(result == NULL);

	/* Test 4 */
	strcpy(string, "XXX_YY_Z");
	delimiters = "";
	result = stringUtilsParseTokens(string, delimiters, &context);
	assert(result != NULL);
	assert(strcmp(result, "XXX_YY_Z") == 0);

	result = stringUtilsParseTokens(NULL, delimiters, &context);
	assert(result == NULL);

	/* Test 5 */
	strcpy(string, "|a||||b|||||||||c||||||||d|");
	delimiters = "|";
	result = stringUtilsParseTokens(string, delimiters, &context);
	assert(result != NULL);
	assert(strcmp(result, "a") == 0);

	result = stringUtilsParseTokens(NULL, delimiters, &context);
	assert(result != NULL);
	assert(strcmp(result, "b") == 0);

	result = stringUtilsParseTokens(NULL, delimiters, &context);
	assert(result != NULL);
	assert(strcmp(result, "c") == 0);

	result = stringUtilsParseTokens(NULL, delimiters, &context);
	assert(result != NULL);
	assert(strcmp(result, "d") == 0);

	result = stringUtilsParseTokens(NULL, delimiters, &context);
	assert(result == NULL);
}

int main(int argc, char** argv) {
	testLeftTrim();
	testRightTrim();
	testTrim();
	testIsDigitOnly();
	testParseTokens();

	return 0;
}
