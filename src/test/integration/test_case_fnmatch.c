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

#include <assert.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>

#include "test/integration_test.h"

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

 	const char* string;
 	const char* pattern;
 	int flags;
 	int fnmatchResult;

 	string = "text";
 	pattern = "?ex*";
 	flags = 0;
	fnmatchResult = fnmatch(pattern, string, flags);
	assert(fnmatchResult == 0);

 	string = "text";
 	pattern = "?ex*1";
 	flags = 0;
	fnmatchResult = fnmatch(pattern, string, flags);
	assert(fnmatchResult == FNM_NOMATCH);

 	string = "text.txt";
 	pattern = "*.txt";
 	flags = 0;
	fnmatchResult = fnmatch(pattern, string, flags);
	assert(fnmatchResult == 0);

 	string = "f0.txt";
 	pattern = "*[!1-9].txt";
 	flags = 0;
	fnmatchResult = fnmatch(pattern, string, flags);
	assert(fnmatchResult == 0);

 	string = "f9.txt";
 	pattern = "*[!1-9].txt";
 	flags = 0;
	fnmatchResult = fnmatch(pattern, string, flags);
	assert(fnmatchResult == FNM_NOMATCH);

	string = "~";
	pattern = "[abcdefghijklmnopqrstuvxyz0123456789ABCDEFGHIJKLMNOPQRSTUVXYZ]";
 	flags = 0;
	fnmatchResult = fnmatch(pattern, string, flags);
	assert(fnmatchResult == FNM_NOMATCH);

	string = "0123456789abcdefgh";
	pattern = "[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][a-z][a-z][a-z][a-z][a-z][a-z][a-z][a-z]";
 	flags = 0;
	fnmatchResult = fnmatch(pattern, string, flags);
	assert(fnmatchResult == 0);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
