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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/command_line_utils.h"

static void testCommandLineUtilsParseArguments1(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = "ab::c:";
	configuration.longOptions = NULL;
	configuration.operationMode = MOVE_NON_OPTIONS_TO_END;

	argc = 5;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "-a";
	argv[2] = "-c";
	argv[3] = "valueForC";
	argv[4] = "-b";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument == NULL);
	assert(result.optionValue == 'a');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument != NULL && strcmp(result.argument, "valueForC") == 0);
	assert(result.optionValue == 'c');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument == NULL);
	assert(result.optionValue == 'b');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 5);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "-a") == 0);
	assert(strcmp(argv[2], "-c") == 0);
	assert(strcmp(argv[3], "valueForC") == 0);
	assert(strcmp(argv[4], "-b") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments2(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = "ab::c:";
	configuration.longOptions = NULL;
	configuration.operationMode = MOVE_NON_OPTIONS_TO_END;

	argc = 6;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "INVALID_1";
	argv[2] = "-a";
	argv[3] = "INVALID_2";
	argv[4] = "INVALID_3";
	argv[5] = "INVALID_4";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument == NULL);
	assert(result.optionValue == 'a');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 2);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "-a") == 0);
	assert(strcmp(argv[2], "INVALID_1") == 0);
	assert(strcmp(argv[3], "INVALID_2") == 0);
	assert(strcmp(argv[4], "INVALID_3") == 0);
	assert(strcmp(argv[5], "INVALID_4") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments3(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = "ab::c:";
	configuration.longOptions = NULL;
	configuration.operationMode = MOVE_NON_OPTIONS_TO_END;

	argc = 11;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "-a";
	argv[2] = "123";
	argv[3] = "40";
	argv[4] = "-c";
	argv[5] = "-b";
	argv[6] = "5";
	argv[7] = "-m";
	argv[8] = "23";
	argv[9] = "-n";
	argv[10] = "amb";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument == NULL);
	assert(result.optionValue == 'a');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument != NULL && strcmp(result.argument, "-b") == 0);
	assert(result.optionValue == 'c');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_UNKNOWN_OPTION);
	assert(result.argument == NULL);
	assert(result.optionValue == 'm');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_UNKNOWN_OPTION);
	assert(result.argument == NULL);
	assert(result.optionValue == 'n');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 6);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "-a") == 0);
	assert(strcmp(argv[2], "-c") == 0);
	assert(strcmp(argv[3], "-b") == 0);
	assert(strcmp(argv[4], "-m") == 0);
	assert(strcmp(argv[5], "-n") == 0);
	assert(strcmp(argv[6], "123") == 0);
	assert(strcmp(argv[7], "40") == 0);
	assert(strcmp(argv[8], "5") == 0);
	assert(strcmp(argv[9], "23") == 0);
	assert(strcmp(argv[10], "amb") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments4(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = "ab::c:";
	configuration.longOptions = NULL;
	configuration.operationMode = MOVE_NON_OPTIONS_TO_END;

	argc = 9;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "-a";
	argv[2] = "222";
	argv[3] = "-x";
	argv[4] = "-n";
	argv[5] = "--";
	argv[6] = "-c";
	argv[7] = "-p";
	argv[8] = "123";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument == NULL);
	assert(result.optionValue == 'a');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_UNKNOWN_OPTION);
	assert(result.argument == NULL);
	assert(result.optionValue == 'x');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_UNKNOWN_OPTION);
	assert(result.argument == NULL);
	assert(result.optionValue == 'n');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 5);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "-a") == 0);
	assert(strcmp(argv[2], "-x") == 0);
	assert(strcmp(argv[3], "-n") == 0);
	assert(strcmp(argv[4], "--") == 0);
	assert(strcmp(argv[5], "222") == 0);
	assert(strcmp(argv[6], "-c") == 0);
	assert(strcmp(argv[7], "-p") == 0);
	assert(strcmp(argv[8], "123") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments5(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	struct option longOptions[] = {
		{"add", required_argument, NULL, 1},
		{"append", no_argument, NULL, 2},
		{"delete", required_argument, NULL, 3},
		{"verbose", no_argument, NULL, 4},
		{"create", required_argument, NULL, 5},
		{"file", required_argument, NULL, 6},
		{0, 0, 0, 0}
	};

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = NULL;
	configuration.longOptions = longOptions;
	configuration.operationMode = MOVE_NON_OPTIONS_TO_END;

	argc = 6;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "--ad";
	argv[2] = "PARAMETER1";
	argv[3] = "--verbose";
	argv[4] = "GARBAGE1";
	argv[5] = "--file";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument != NULL && strcmp(result.argument, "PARAMETER1") == 0);
	assert(result.optionValue == 1);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument == NULL);
	assert(result.optionValue == 4);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_MISSING_REQUIRED_PARAMETER);
	assert(result.argument == NULL);
	assert(result.optionValue == 6);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 5);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "--ad") == 0);
	assert(strcmp(argv[2], "PARAMETER1") == 0);
	assert(strcmp(argv[3], "--verbose") == 0);
	assert(strcmp(argv[4], "--file") == 0);
	assert(strcmp(argv[5], "GARBAGE1") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments6(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	struct option longOptions[] = {
		{"opt1", optional_argument, NULL, 1},
		{"opt2", no_argument, NULL, 2},
		{"opt3", required_argument, NULL, 3},
		{"opt4", no_argument, NULL, 4},
		{0, 0, 0, 0}
	};

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = "a::b:c::o";
	configuration.longOptions = longOptions;
	configuration.operationMode = MOVE_NON_OPTIONS_TO_END;

	argc = 8;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "GARBAGE1";
	argv[2] = "GARBAGE2";
	argv[3] = "--opt1";
	argv[4] = "-a";
	argv[5] = "--opt3";
	argv[6] = "PARAMETER1";
	argv[7] = "-o";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument == NULL);
	assert(result.optionValue == 1);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument == NULL);
	assert(result.optionValue == 'a');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument != NULL && strcmp(result.argument, "PARAMETER1") == 0);
	assert(result.optionValue == 3);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument == NULL);
	assert(result.optionValue == 'o');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 6);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "--opt1") == 0);
	assert(strcmp(argv[2], "-a") == 0);
	assert(strcmp(argv[3], "--opt3") == 0);
	assert(strcmp(argv[4], "PARAMETER1") == 0);
	assert(strcmp(argv[5], "-o") == 0);
	assert(strcmp(argv[6], "GARBAGE1") == 0);
	assert(strcmp(argv[7], "GARBAGE2") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments7(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	struct option longOptions[] = {
		{"opt1", optional_argument, NULL, 1},
		{"opt2", no_argument, NULL, 2},
		{"opt3", required_argument, NULL, 3},
		{"opt4", no_argument, NULL, 4},
		{0, 0, 0, 0}
	};

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = "a::b:c::o";
	configuration.longOptions = longOptions;
	configuration.operationMode = MOVE_NON_OPTIONS_TO_END;

	argc = 9;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "GARBAGE1";
	argv[2] = "GARBAGE2";
	argv[3] = "--o";
	argv[4] = "--";
	argv[5] = "-a";
	argv[6] = "--opt3";
	argv[7] = "PARAMETER1";
	argv[8] = "-o";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_MORE_THAN_ONE_OPTION_WITH_PREFIX);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 3);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "--o") == 0);
	assert(strcmp(argv[2], "--") == 0);
	assert(strcmp(argv[3], "GARBAGE1") == 0);
	assert(strcmp(argv[4], "GARBAGE2") == 0);
	assert(strcmp(argv[5], "-a") == 0);
	assert(strcmp(argv[6], "--opt3") == 0);
	assert(strcmp(argv[7], "PARAMETER1") == 0);
	assert(strcmp(argv[8], "-o") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments8(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	struct option longOptions[] = {
		{"opt1", optional_argument, NULL, 10},
		{"opt2", no_argument, NULL, 20},
		{"opt3", required_argument, NULL, 30},
		{"opt4", no_argument, NULL, 40},
		{0, 0, 0, 0}
	};

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = "a::b:c::o";
	configuration.longOptions = longOptions;
	configuration.operationMode = HANDLE_NON_OPTIONS_AS_ARGUMENT_OF_ONE;

	argc = 8;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "GARBAGE1";
	argv[2] = "GARBAGE2";
	argv[3] = "--o";
	argv[4] = "-a";
	argv[5] = "--opt3";
	argv[6] = "PARAMETER1";
	argv[7] = "-o";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument != NULL && strcmp(result.argument, "GARBAGE1") == 0);
	assert(result.optionValue == 1);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument != NULL && strcmp(result.argument, "GARBAGE2") == 0);
	assert(result.optionValue == 1);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_MORE_THAN_ONE_OPTION_WITH_PREFIX);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument == NULL);
	assert(result.optionValue == 'a');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument != NULL && strcmp(result.argument, "PARAMETER1") == 0);
	assert(result.optionValue == 30);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument == NULL);
	assert(result.optionValue == 'o');

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 8);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "GARBAGE1") == 0);
	assert(strcmp(argv[2], "GARBAGE2") == 0);
	assert(strcmp(argv[3], "--o") == 0);
	assert(strcmp(argv[4], "-a") == 0);
	assert(strcmp(argv[5], "--opt3") == 0);
	assert(strcmp(argv[6], "PARAMETER1") == 0);
	assert(strcmp(argv[7], "-o") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments9(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	struct option longOptions[] = {
		{"opt1", optional_argument, NULL, 10},
		{"opt2", no_argument, NULL, 20},
		{"opt3", required_argument, NULL, 30},
		{"opt4", no_argument, NULL, 40},
		{0, 0, 0, 0}
	};

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = "";
	configuration.longOptions = longOptions;
	configuration.operationMode = HANDLE_NON_OPTIONS_AS_ARGUMENT_OF_ONE;

	argc = 8;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "GARBAGE1";
	argv[2] = "GARBAGE2";
	argv[3] = "--";
	argv[4] = "-a";
	argv[5] = "--opt3";
	argv[6] = "PARAMETER1";
	argv[7] = "-o";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument != NULL && strcmp(result.argument, "GARBAGE1") == 0);
	assert(result.optionValue == 1);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument != NULL && strcmp(result.argument, "GARBAGE2") == 0);
	assert(result.optionValue == 1);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 4);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "GARBAGE1") == 0);
	assert(strcmp(argv[2], "GARBAGE2") == 0);
	assert(strcmp(argv[3], "--") == 0);
	assert(strcmp(argv[4], "-a") == 0);
	assert(strcmp(argv[5], "--opt3") == 0);
	assert(strcmp(argv[6], "PARAMETER1") == 0);
	assert(strcmp(argv[7], "-o") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments10(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	struct option longOptions[] = {
		{"opt1", optional_argument, NULL, 10},
		{"opt2", no_argument, NULL, 20},
		{"opt3", required_argument, NULL, 30},
		{"opt4", no_argument, NULL, 40},
		{0, 0, 0, 0}
	};

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = NULL;
	configuration.longOptions = longOptions;
	configuration.operationMode = MOVE_NON_OPTIONS_TO_END;

	argc = 8;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "GARBAGE1";
	argv[2] = "--opt1=ABC";
	argv[3] = "--opt4=123";
	argv[4] = "GARBAGE2";
	argv[5] = "--";
	argv[6] = "GARBAGE3";
	argv[7] = "--opt3=xyz";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument != NULL && strcmp(result.argument, "ABC") == 0);
	assert(result.optionValue == 10);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_PARAMETER_ALLOWED);
	assert(result.argument != NULL && strcmp(result.argument, "123") == 0);
	assert(result.optionValue == 40);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 4);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "--opt1=ABC") == 0);
	assert(strcmp(argv[2], "--opt4=123") == 0);
	assert(strcmp(argv[3], "--") == 0);
	assert(strcmp(argv[4], "GARBAGE1") == 0);
	assert(strcmp(argv[5], "GARBAGE2") == 0);
	assert(strcmp(argv[6], "GARBAGE3") == 0);
	assert(strcmp(argv[7], "--opt3=xyz") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments11(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = "ab::c:";
	configuration.longOptions = NULL;
	configuration.operationMode = STOP_ON_FIRST_NON_OPTION;

	argc = 5;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "A";
	argv[2] = "-c";
	argv[3] = "valueForC";
	argv[4] = "-b";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 1);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "A") == 0);
	assert(strcmp(argv[2], "-c") == 0);
	assert(strcmp(argv[3], "valueForC") == 0);
	assert(strcmp(argv[4], "-b") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments12(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	struct option longOptions[] = {
		{"opt1", optional_argument, NULL, 10},
		{"opt2", no_argument, NULL, 20},
		{"opt3", required_argument, NULL, 30},
		{"opt4", no_argument, NULL, 40},
		{0, 0, 0, 0}
	};

	commandLineUtilsInitializeContext(&context, 0, 1);
	configuration.shortOptionCharacters = NULL;
	configuration.longOptions = longOptions;
	configuration.operationMode = HANDLE_NON_OPTIONS_AS_ARGUMENT_OF_ONE;

	argc = 3;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "--opt5";
	argv[1] = "--opt3";
	argv[2] = "VALUE1";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_UNKNOWN_OPTION);
	assert(result.argument == NULL);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.argument != NULL && strcmp(result.argument, "VALUE1") == 0);
	assert(result.optionValue == 30);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);
	assert(result.argument == NULL);
	assert(result.optionValue == 0);

	assert(context.nextArgumentIndex == 3);

	assert(strcmp(argv[0], "--opt5") == 0);
	assert(strcmp(argv[1], "--opt3") == 0);
	assert(strcmp(argv[2], "VALUE1") == 0);

	free(argv);
}

static void testCommandLineUtilsParseArguments13(void) {
	int argc;
	const char** argv = NULL;
	struct CommandLineArgumentsParsingConfiguration configuration;
	struct CommandLineArgumentsParsingContext context;
	struct CommandLineArgumentsParsingResult result;

	commandLineUtilsInitializeContext(&context, 1, 1);
	configuration.shortOptionCharacters = "ab::c:defgh";
	configuration.longOptions = NULL;
	configuration.operationMode = MOVE_NON_OPTIONS_TO_END;

	argc = 7;
	argv = malloc(sizeof(char*) * argc);
	argv[0] = "executable";
	argv[1] = "A";
	argv[2] = "-defbvalueForB";
	argv[3] = "-ab";
	argv[4] = "-gh";
	argv[5] = "-c";
	argv[6] = "valueForC";

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.optionValue == 'd');
	assert(result.argument == NULL);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.optionValue == 'e');
	assert(result.argument == NULL);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.optionValue == 'f');
	assert(result.argument == NULL);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.optionValue == 'b');
	assert(strcmp(result.argument, "valueForB") == 0);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.optionValue == 'a');
	assert(result.argument == NULL);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.optionValue == 'b');
	assert(result.argument == NULL);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.optionValue == 'g');
	assert(result.argument == NULL);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.optionValue == 'h');
	assert(result.argument == NULL);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_ERROR);
	assert(result.optionValue == 'c');
	assert(strcmp(result.argument, "valueForC") == 0);

	commandLineUtilsParseArguments(argc, argv, &configuration, &context, &result);
	assert(result.errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS);

	assert(context.nextArgumentIndex == 6);

	assert(strcmp(argv[0], "executable") == 0);
	assert(strcmp(argv[1], "-defbvalueForB") == 0);
	assert(strcmp(argv[2], "-ab") == 0);
	assert(strcmp(argv[3], "-gh") == 0);
	assert(strcmp(argv[4], "-c") == 0);
	assert(strcmp(argv[5], "valueForC") == 0);
	assert(strcmp(argv[6], "A") == 0);

	free(argv);
}

void testCommandLineUtilsSplit1(void) {
	static const int ARGUMENTS_LENGTH = 8;
	static const size_t ARGUMENTS_BUFFER_SIZE = 32;

	int argumentsCount = 0;
	const char* arguments[ARGUMENTS_LENGTH];
	char argumentsBuffer[ARGUMENTS_BUFFER_SIZE];

	commandLineUtilsSplit("   a  -b   c   123456789  DDDD", argumentsBuffer, ARGUMENTS_BUFFER_SIZE, arguments, ARGUMENTS_LENGTH, &argumentsCount);

	assert(argumentsCount == 5);
	assert(arguments[0] != NULL && strcmp(arguments[0], "a") == 0);
	assert(arguments[1] != NULL && strcmp(arguments[1], "-b") == 0);
	assert(arguments[2] != NULL && strcmp(arguments[2], "c") == 0);
	assert(arguments[3] != NULL && strcmp(arguments[3], "123456789") == 0);
	assert(arguments[4] != NULL && strcmp(arguments[4], "DDDD") == 0);
}

void testCommandLineUtilsSplit2(void) {
	static const int ARGUMENTS_LENGTH = 4;
	static const size_t ARGUMENTS_BUFFER_SIZE = 4;

	int argumentsCount = 0;
	const char* arguments[ARGUMENTS_LENGTH];
	char argumentsBuffer[ARGUMENTS_BUFFER_SIZE];

	commandLineUtilsSplit("AAA BBB CCC", argumentsBuffer, ARGUMENTS_BUFFER_SIZE, arguments, ARGUMENTS_LENGTH, &argumentsCount);

	assert(argumentsCount == 1);
	assert(arguments[0] != NULL && strcmp(arguments[0], "AAA") == 0);
}

void testCommandLineUtilsSplit3(void) {
	static const int ARGUMENTS_LENGTH = 1;
	static const size_t ARGUMENTS_BUFFER_SIZE = 128;

	int argumentsCount = 0;
	const char* arguments[ARGUMENTS_LENGTH];
	char argumentsBuffer[ARGUMENTS_BUFFER_SIZE];

	commandLineUtilsSplit("AAA BBB CCC", argumentsBuffer, ARGUMENTS_BUFFER_SIZE, arguments, ARGUMENTS_LENGTH, &argumentsCount);

	assert(argumentsCount == 1);
	assert(arguments[0] != NULL && strcmp(arguments[0], "AAA") == 0);
}

void testCommandLineUtilsSplit4(void) {
	static const int ARGUMENTS_LENGTH = 2;
	static const size_t ARGUMENTS_BUFFER_SIZE = 128;

	int argumentsCount = 0;
	const char* arguments[ARGUMENTS_LENGTH];
	char argumentsBuffer[ARGUMENTS_BUFFER_SIZE];

	commandLineUtilsSplit("'AAA\"BBB'  \" CCC'CCC \"", argumentsBuffer, ARGUMENTS_BUFFER_SIZE, arguments, ARGUMENTS_LENGTH, &argumentsCount);

	assert(argumentsCount == 2);
	assert(arguments[0] != NULL && strcmp(arguments[0], "AAA\"BBB") == 0);
	assert(arguments[1] != NULL && strcmp(arguments[1], " CCC'CCC ") == 0);
}

void testCommandLineUtilsSplit5(void) {
	static const int ARGUMENTS_LENGTH = 8;
	static const size_t ARGUMENTS_BUFFER_SIZE = 128;

	int argumentsCount = 0;
	const char* arguments[ARGUMENTS_LENGTH];
	char argumentsBuffer[ARGUMENTS_BUFFER_SIZE];

	commandLineUtilsSplit(" '  '  \" \" A B --C'aa'  ", argumentsBuffer, ARGUMENTS_BUFFER_SIZE, arguments, ARGUMENTS_LENGTH, &argumentsCount);

	assert(argumentsCount == 6);
	assert(arguments[0] != NULL && strcmp(arguments[0], "  ") == 0);
	assert(arguments[1] != NULL && strcmp(arguments[1], " ") == 0);
	assert(arguments[2] != NULL && strcmp(arguments[2], "A") == 0);
	assert(arguments[3] != NULL && strcmp(arguments[3], "B") == 0);
	assert(arguments[4] != NULL && strcmp(arguments[4], "--C") == 0);
	assert(arguments[5] != NULL && strcmp(arguments[5], "aa") == 0);
}

void testCommandLineUtilsSplit6(void) {
	static const int ARGUMENTS_LENGTH = 16;
	static const size_t ARGUMENTS_BUFFER_SIZE = 256;

	int argumentsCount = 0;
	const char* arguments[ARGUMENTS_LENGTH];
	char argumentsBuffer[ARGUMENTS_BUFFER_SIZE];

	commandLineUtilsSplit("a'\"b' \t\n  x\"y    \"'XX\"XX'   \"\"fff'sss", argumentsBuffer, ARGUMENTS_BUFFER_SIZE, arguments, ARGUMENTS_LENGTH, &argumentsCount);

	assert(argumentsCount == 6);
	assert(arguments[0] != NULL && strcmp(arguments[0], "a") == 0);
	assert(arguments[1] != NULL && strcmp(arguments[1], "\"b") == 0);
	assert(arguments[2] != NULL && strcmp(arguments[2], "x") == 0);
	assert(arguments[3] != NULL && strcmp(arguments[3], "y    ") == 0);
	assert(arguments[4] != NULL && strcmp(arguments[4], "XX\"XX") == 0);
	assert(arguments[5] != NULL && strcmp(arguments[5], "fff") == 0);
}

void testCommandLineUtilsSplit7(void) {
	static const int ARGUMENTS_LENGTH = 1;
	static const size_t ARGUMENTS_BUFFER_SIZE = 1;

	int argumentsCount = 0;
	const char* arguments[ARGUMENTS_LENGTH];
	char argumentsBuffer[ARGUMENTS_BUFFER_SIZE];

	commandLineUtilsSplit("AAAAAAA", argumentsBuffer, ARGUMENTS_BUFFER_SIZE, arguments, ARGUMENTS_LENGTH, &argumentsCount);

	assert(argumentsCount == 0);
}

void testCommandLineUtilsSplit8(void) {
	static const int ARGUMENTS_LENGTH = 8;
	static const size_t ARGUMENTS_BUFFER_SIZE = 16;

	int argumentsCount = 0;
	const char* arguments[ARGUMENTS_LENGTH];
	char argumentsBuffer[ARGUMENTS_BUFFER_SIZE];

	commandLineUtilsSplit("AAAA BBBB CC D EEEEEE", argumentsBuffer, ARGUMENTS_BUFFER_SIZE, arguments, ARGUMENTS_LENGTH, &argumentsCount);

	assert(argumentsCount == 4);
	assert(arguments[0] != NULL && strcmp(arguments[0], "AAAA") == 0);
	assert(arguments[1] != NULL && strcmp(arguments[1], "BBBB") == 0);
	assert(arguments[2] != NULL && strcmp(arguments[2], "CC") == 0);
	assert(arguments[3] != NULL && strcmp(arguments[3], "D") == 0);
}

int main(int argc, char** argv) {
	testCommandLineUtilsParseArguments1();
	testCommandLineUtilsParseArguments2();
	testCommandLineUtilsParseArguments3();
	testCommandLineUtilsParseArguments4();
	testCommandLineUtilsParseArguments5();
	testCommandLineUtilsParseArguments6();
	testCommandLineUtilsParseArguments7();
	testCommandLineUtilsParseArguments8();
	testCommandLineUtilsParseArguments9();
	testCommandLineUtilsParseArguments10();
	testCommandLineUtilsParseArguments11();
	testCommandLineUtilsParseArguments12();
	testCommandLineUtilsParseArguments13();

	testCommandLineUtilsSplit1();
	testCommandLineUtilsSplit2();
	testCommandLineUtilsSplit3();
	testCommandLineUtilsSplit4();
	testCommandLineUtilsSplit5();
	testCommandLineUtilsSplit6();
	testCommandLineUtilsSplit7();
	testCommandLineUtilsSplit8();
	return 0;
}
