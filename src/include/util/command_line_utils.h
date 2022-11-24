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

#ifndef COMMAND_LINE_UTILS_H
	#define COMMAND_LINE_UTILS_H

	#include <stdbool.h>
	#include <stddef.h>

	#define no_argument 0
   #define required_argument 1
  	#define optional_argument 2

	struct option {
		const char* name;
		int has_arg;
		int* flag;
		int val;
	};

	struct CommandLineArgumentsParsingContext {
		int nextArgumentIndex;
		int nextArgumentCharacterIndex;
	};

	enum CommandLineArgumentsParsingOperationMode {
		STOP_ON_FIRST_NON_OPTION,
		MOVE_NON_OPTIONS_TO_END,
		HANDLE_NON_OPTIONS_AS_ARGUMENT_OF_ONE
	};

	enum CommandLineArgumentsParsingErrorType {
		COMMAND_LINE_UTILS_NO_ERROR,
		COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS,
		COMMAND_LINE_UTILS_MISSING_REQUIRED_PARAMETER,
		COMMAND_LINE_UTILS_NO_PARAMETER_ALLOWED,
		COMMAND_LINE_UTILS_UNKNOWN_OPTION,
		COMMAND_LINE_UTILS_MORE_THAN_ONE_OPTION_WITH_PREFIX
	};

	struct CommandLineArgumentsParsingConfiguration {
		enum CommandLineArgumentsParsingOperationMode operationMode;
		const char* shortOptionCharacters;
		const struct option* longOptions;
	};

	struct CommandLineArgumentsParsingResult {
		enum CommandLineArgumentsParsingErrorType errorType;
		const char* argument;
		int optionValue;
		const struct option* option1;
		const struct option* option2;
	};

	void commandLineUtilsParseArguments(int argc, const char** argv, struct CommandLineArgumentsParsingConfiguration* configuration,
			struct CommandLineArgumentsParsingContext* context, struct CommandLineArgumentsParsingResult* result);

	void commandLineUtilsInitializeContext(struct CommandLineArgumentsParsingContext* context, int nextArgumentIndex, int nextArgumentCharacterIndex);

	void commandLineUtilsSplit(const char* rawCommandLine,
			char* argumentsBuffer, size_t argumentsBufferSize,
			const char** arguments, int argumentsLength,
			int* argumentCount);

#endif
