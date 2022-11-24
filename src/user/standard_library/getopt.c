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
#include <error.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "util/command_line_utils.h"

char *optarg;
int optind = 1;
int opterr = 1;
int optopt;

static int nextArgumentCharacterIndex = 1;

int getopt_long(int argc, char* const argv[], const char* shortOptionCharacters, const struct option* longOptions, int* selectedLongOptionIndex) {
	struct CommandLineArgumentsParsingConfiguration configuration;
	memset(&configuration, 0, sizeof(struct CommandLineArgumentsParsingConfiguration));
	bool enhancedReturn = false;
	if (shortOptionCharacters != NULL && (shortOptionCharacters[0] == '+' || shortOptionCharacters[0] == '-')) {
		configuration.operationMode = shortOptionCharacters[0] == '-' ? HANDLE_NON_OPTIONS_AS_ARGUMENT_OF_ONE : STOP_ON_FIRST_NON_OPTION;
		if (strlen(shortOptionCharacters) >= 2 && shortOptionCharacters[1] == ':') {
			configuration.shortOptionCharacters = shortOptionCharacters + 2;
			enhancedReturn = true;
		} else {
			configuration.shortOptionCharacters = shortOptionCharacters + 1;
		}

	} else {
		if (shortOptionCharacters != NULL && shortOptionCharacters[0] == ':') {
			enhancedReturn = true;
			configuration.shortOptionCharacters = shortOptionCharacters + 1;
		} else {
			configuration.shortOptionCharacters = shortOptionCharacters;
		}
		configuration.operationMode = MOVE_NON_OPTIONS_TO_END;
	}
	configuration.longOptions = longOptions;

	struct CommandLineArgumentsParsingContext context;
	if (optind == 0) {
		optind = 1;
		nextArgumentCharacterIndex = 1;
	}
	commandLineUtilsInitializeContext(&context, optind, nextArgumentCharacterIndex);

	int result = 0;
	struct CommandLineArgumentsParsingResult parsingResult;
	commandLineUtilsParseArguments(argc, (const char**) argv, &configuration, &context, &parsingResult);
	optarg = NULL;
	switch (parsingResult.errorType) {
		case COMMAND_LINE_UTILS_NO_ERROR:
			optarg = (char*) parsingResult.argument;
			result = parsingResult.optionValue;
			break;

		case COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS:
			result = -1;
			break;

		case COMMAND_LINE_UTILS_MISSING_REQUIRED_PARAMETER:
			if (enhancedReturn) {
				result = ':';
			} else {
				result = '?';
			}
			optopt = parsingResult.optionValue;
			if (opterr > 0 && !enhancedReturn) {
				error(0, 0, "option '%c' requires an argument", parsingResult.optionValue);
			}
			break;

		case COMMAND_LINE_UTILS_NO_PARAMETER_ALLOWED:
			if (enhancedReturn) {
				result = ':';
			} else {
				result = '?';
			}
			optopt = parsingResult.optionValue;
			if (opterr > 0 && !enhancedReturn) {
				error(0, 0, "option '%c' does not permit an argument (%s)", parsingResult.optionValue, parsingResult.argument);
			}
			break;

		case COMMAND_LINE_UTILS_UNKNOWN_OPTION:
			result = '?';
			optopt = parsingResult.optionValue;
			if (opterr > 0 && !enhancedReturn) {
				error(0, 0, "option '%c' is not known\n", parsingResult.optionValue);
			}
			break;

		case COMMAND_LINE_UTILS_MORE_THAN_ONE_OPTION_WITH_PREFIX:
			result = '?';
			optopt = parsingResult.optionValue;
			if (opterr > 0 && !enhancedReturn) {
				error(0, 0, "option '%s' is ambiguous as it has at least two possibilities: '%s' and '%s'", argv[optind], parsingResult.option1,
						parsingResult.option2);
			}
			break;
	}

	if (selectedLongOptionIndex != NULL && parsingResult.option1 != NULL) {
		for (int i = 0;; i++) {
			if (&longOptions[i] == parsingResult.option1) {
				*selectedLongOptionIndex = i;
				break;
			}
		}
	}

	optind = context.nextArgumentIndex;

	return result;
}

int getopt(int argc, char* const argv[], const char* shortOptionCharacters) {
	return getopt_long(argc, argv, shortOptionCharacters, NULL, NULL);
}
