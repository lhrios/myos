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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <myos.h>

#include "test/integration_test.h"

static void test1() {
	int argc;
	char** argv = NULL;

	int flag;
	struct option longOptions[] = {
		{"add", required_argument, &flag, 1},
		{"append", no_argument, &flag, 2},
		{"delete", required_argument, &flag, 3},
		{"verbose", no_argument, &flag, 4},
		{"create", required_argument, &flag, 5},
		{"file", required_argument, &flag, 6},
		{0, 0, 0, 0}
	};

	argc = 8;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "--add";
	argv[2] = "PARAMETER1";
	argv[3] = "--create";
	argv[4] = "PARAMETER2";
	argv[5] = "--verbose";
	argv[6] = "--file";
	argv[7] = "PARAMETER3";

	int selectedLongOptionIndex;
	int result;

	result = getopt_long(argc, argv, NULL, longOptions, &selectedLongOptionIndex);
	assert(result == 0);
	assert(strcmp(optarg, "PARAMETER1") == 0);
	assert(selectedLongOptionIndex == 0);
	assert(flag == 1);

	result = getopt_long(argc, argv, NULL, longOptions, &selectedLongOptionIndex);
	assert(result == 0);
	assert(strcmp(optarg, "PARAMETER2") == 0);
	assert(selectedLongOptionIndex == 4);
	assert(flag == 5);

	result = getopt_long(argc, argv, NULL, longOptions, &selectedLongOptionIndex);
	assert(result == 0);
	assert(optarg == NULL);
	assert(selectedLongOptionIndex == 3);
	assert(flag == 4);

	result = getopt_long(argc, argv, NULL, longOptions, &selectedLongOptionIndex);
	assert(result == 0);
	assert(strcmp(optarg, "PARAMETER3") == 0);
	assert(selectedLongOptionIndex == 5);
	assert(flag == 6);

	assert(optind == 8);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "--add") == 0);
	assert(strcmp(argv[2], "PARAMETER1") == 0);
	assert(strcmp(argv[3], "--create") == 0);
	assert(strcmp(argv[4], "PARAMETER2") == 0);
	assert(strcmp(argv[5], "--verbose") == 0);
	assert(strcmp(argv[6], "--file") == 0);
	assert(strcmp(argv[7], "PARAMETER3") == 0);

	free(argv);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	test1();

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
