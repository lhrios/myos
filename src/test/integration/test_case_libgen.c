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
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test/integration_test.h"

static void testDirNameAndBaseName(const char* path, const char* expectedDirName, const char* expectedBaseName) {
	char* dirName = path == NULL ? dirname(NULL) : dirname(strdup(path));
	assert(strcmp(expectedDirName, dirName) == 0);

	char* baseName = path == NULL ? basename(NULL) : basename(strdup(path));
	assert(strcmp(expectedBaseName, baseName) == 0);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	testDirNameAndBaseName("///usr//bin/a.txt", "///usr//bin", "a.txt");
	testDirNameAndBaseName("////a.txt", "/", "a.txt");
	testDirNameAndBaseName("a", ".", "a");
	testDirNameAndBaseName(NULL, ".", ".");
	testDirNameAndBaseName("/tmp/", "/", "tmp");
	testDirNameAndBaseName(".", ".", ".");
	testDirNameAndBaseName("..", ".", "..");
	testDirNameAndBaseName("/usr/../bin/a.txt", "/usr/../bin", "a.txt");
	testDirNameAndBaseName("/", "/", "/");
	testDirNameAndBaseName("./", ".", ".");
	testDirNameAndBaseName("../", ".", "..");
	testDirNameAndBaseName("../a", "..", "a");
	testDirNameAndBaseName("/tmp", "/", "tmp");
	testDirNameAndBaseName("////", "/", "/");
	testDirNameAndBaseName("   ", ".", "   ");
	testDirNameAndBaseName("", ".", ".");
	testDirNameAndBaseName("/usr/../bin/a.txt", "/usr/../bin", "a.txt");
	testDirNameAndBaseName("/a////b////", "/a", "b");

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
