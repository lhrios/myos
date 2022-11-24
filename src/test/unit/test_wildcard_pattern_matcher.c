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
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "user/util/wildcard_pattern_matcher.h"

static void test(const char* string, const char* pattern, int flags, bool expectedResult) {
	bool fnmatchResult = fnmatch(pattern, string, flags) == 0;
	if (fnmatchResult != expectedResult) {
		fprintf(stderr, "Using \"fnmatch\"\n");
		fprintf(stderr, "string=\"%s\"\npattern=\"%s\"\nexpected=%d\nfound=%d\n", string, pattern, expectedResult, fnmatchResult);
		assert(false);
	}

	bool wildcardPatternMatcherResult = wildcardPatternMatcherCompileAndMatch(pattern, string, flags) == 0;
	if (wildcardPatternMatcherResult != expectedResult) {
		fprintf(stderr, "Using \"wildcardPatternMatcherMatch\"\n");
		fprintf(stderr, "string=\"%s\"\npattern=\"%s\"\nexpected=%d\nfound=%d\n", string, pattern, expectedResult, wildcardPatternMatcherResult);
		assert(false);
	}
}

int main(int argc, char** argv) {
	const char* string;
	const char* pattern;

	string = ".";
	pattern = "*";
	test(string, pattern, 0, true);

	string = "..";
	pattern = "*";
	test(string, pattern, FNM_PERIOD, false);

 	string = "text";
 	pattern = "?[![:alpha:]-[:digit:]][]-]tex*t[A-Fa-f0-9--0][[?*\\][!]a-][!][]";
	test(string, pattern, 0, false);

	string = "text";
	pattern = "tex*t";
	test(string, pattern, 0, true);

	string = "text";
	pattern = "text123";
	test(string, pattern, 0, false);

	string = "text.txt";
	pattern = "*.txt";
	test(string, pattern, 0, true);

	string = "text.txt";
	pattern = "t*.txt";
	test(string, pattern, 0, true);

	string = "text.txt";
	pattern = "a*.txt";
	test(string, pattern, 0, false);

	string = "text.txt";
	pattern = "t???.txt";
	test(string, pattern, 0, true);

	string = "text.txt";
	pattern = "t??.txt";
	test(string, pattern, 0, false);

	string = "text.txt";
	pattern = "*.*";
	test(string, pattern, 0, true);

	string = "text.txt";
	pattern = "*";
	test(string, pattern, 0, true);

	string = "text.txt";
	pattern = "text?*";
	test(string, pattern, 0, true);

	string = "text.txt";
	pattern = "text.txt*";
	test(string, pattern, 0, true);

	string = "muchLongerFileName.extension";
	pattern = "*Name*";
	test(string, pattern, 0, true);

	string = "";
	pattern = "*";
	test(string, pattern, 0, true);

	string = "file";
	pattern = "";
	test(string, pattern, 0, false);

	string = "text.txt";
	pattern = "[[:alpha:]]*[[:punct:]]txt";
	test(string, pattern, 0, true);

	string = "text.txt";
	pattern = "[[:digit:]]*[[:punct:]]txt";
	test(string, pattern, 0, false);

	string = "3333.444";
	pattern = "[3-4][3-4][3-4][3-4].[3-4][3-4][0-9]";
	test(string, pattern, 0, true);

	string = "3333.444";
	pattern = "[3-4][3-4][3-4][3-4].[3-4][3-4][0-9]";
	test(string, pattern, 0, true);

	string = "3333.444";
	pattern = "[3-4]*";
	test(string, pattern, 0, true);

	string = "3333.444";
	pattern = "[4-3]*";
	test(string, pattern, 0, false);

	string = "T123";
	pattern = "[[:lower:]]*";
	test(string, pattern, 0, false);

	string = "t123";
	pattern = "[[:lower:]]*[0-9]";
	test(string, pattern, 0, true);

	string = "\\?*";
	pattern = "[\\\\][?][*]";
	test(string, pattern, 0, true);

	string = "?*\\";
	pattern = "\\?\\*\\\\";
	test(string, pattern, 0, true);

	string = "?*\\";
	pattern = "[!a]*";
	test(string, pattern, 0, true);

	string = "course";
	pattern = "[abc]*[![:digit:]]";
	test(string, pattern, 0, true);

	string = "cours3";
	pattern = "[abc]*[![:digit:]]";
	test(string, pattern, 0, false);

	string = "\n ";
	pattern = "[[:space:]][[:blank:]]";
	test(string, pattern, 0, true);

	string = "A";
	pattern = "[a-z]";
	test(string, pattern, 0, false);

	string = "a";
	pattern = "[[:unknown:]]";
	test(string, pattern, 0, false);

	string = "!a!";
	pattern = "[a!][a!][\\!]";
	test(string, pattern, 0, true);

	string = "abc-]";
	pattern = "abc[]-][]-]";
	test(string, pattern, 0, true);

	string = "-.0/";
	pattern = "[--0][--0][--0][--0]";
	test(string, pattern, 0, true);

	string = "1";
	pattern = "[--0]";
	test(string, pattern, 0, false);

	string = "a]-";
	pattern = "[!]a-][!]a-][!]a-]";
	test(string, pattern, 0, false);

	string = "d";
	pattern = "[ab\\]d]";
	test(string, pattern, 0, true);

	string = "\\abc]";
	pattern = "[\\\\]abc]";
	test(string, pattern, 0, true);

	string = "\\*?[";
	pattern = "[[?*\\\\][[?*\\\\][[?*\\\\][[?*\\\\]";
	test(string, pattern, 0, true);

	string = "\\";
	pattern = "[[?*\\]a]";
	test(string, pattern, 0, false);

	string = "a";
	pattern = "[[?*\\]a]";
	test(string, pattern, 0, true);

	string = "veryButVeryVeryLongStringWithTooManyCharactersButNoneOfThemAreNumbers";
	pattern = "veryButVeryVeryLongStringWithTooManyCharactersButNoneOfThemAreNumbers";
	test(string, pattern, 0, true);

	string = "1";
	pattern = "[abcdefghijklmnopqrstuvxyz0123456789ABCDEFGHIJKLMNOPQRSTUVXYZ]";
	test(string, pattern, 0, true);

	string = "0123456789abcdefgh";
	pattern = "[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][a-z][a-z][a-z][a-z][a-z][a-z][a-z][a-z]";
	test(string, pattern, 0, true);

 	string = "3333";
 	pattern = "[3-4][3-4][3-4][3-4]";
 	test(string, pattern, 0, true);

 	string = "0";
 	pattern = "[0-11-22-33-44-55-66-77-88-9a-bb-cc-dd-ee-ff-gg-hh-ii-jj-kk-ll-mm-nn-oo-pp-qq-rr-ss-tt-uu-vv-xx-z]";
 	test(string, pattern, 0, true);

 	string = "test1";
 	pattern = "test[[:digit:]]";
 	test(string, pattern, 0, true);

	string = "test\\";
 	pattern = "test\\*";
 	test(string, pattern, FNM_NOESCAPE, true);

	string = "test\\a";
 	pattern = "test[\\bcd]a";
 	test(string, pattern, FNM_NOESCAPE, true);

	string = "/a/b/c";
 	pattern = "*";
 	test(string, pattern, FNM_PATHNAME, false);

	string = "/a/b/c";
 	pattern = "/?/*?/c";
 	test(string, pattern, FNM_PATHNAME, true);

	string = "./a/.b/c";
 	pattern = "./?/*?/c";
 	test(string, pattern, FNM_PERIOD, true);

	string = "./a/.b.TXT/c";
 	pattern = "./?/.*?TXT/c";
 	test(string, pattern, FNM_PERIOD | FNM_PATHNAME, true);

	string = "/A/.B/.c.";
 	pattern = "/?/.*/.[a-z]?*";
 	test(string, pattern, FNM_PERIOD | FNM_PATHNAME, true);

	string = "AAAA";
 	pattern = "[[:upper:]]a[a-z][a]";
 	test(string, pattern, FNM_CASEFOLD, true);

	string = "aaaa";
 	pattern = "[[:lower:]][A-Z][A]A";
 	test(string, pattern, FNM_CASEFOLD, true);

	string = "abc/def";
 	pattern = "[a-c][a-c][a-c]";
 	test(string, pattern, FNM_LEADING_DIR, true);

	string = "/abc/def";
 	pattern = "";
 	test(string, pattern, FNM_LEADING_DIR, true);

	string = "/abc/def";
 	pattern = "*";
 	test(string, pattern, FNM_LEADING_DIR, true);

	string = "abcdef";
 	pattern = "*c*e?";
 	test(string, pattern, FNM_LEADING_DIR, true);

	string = "veryButVeryVeryLongStringWithTooManyCharactersButNoneOfThemAreNumbers";
	pattern = "*veryButVeryVeryLongStringWithTooManyCharactersButNoneOfThemAreNumbers";
	test(string, pattern, 0, true);

	return EXIT_SUCCESS;
}
