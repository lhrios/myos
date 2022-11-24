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
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/command_line_utils.h"
#include "util/string_stream_writer.h"

static bool isLongOption(const char* argument) {
	return strlen(argument) >= 3 && argument[0] == '-' && argument[1] == '-';
}

static bool shortOptionExists(char option, const char* shortOptionCharacters, bool* admitsArgument, bool* isArgumentOptional) {
	if (shortOptionCharacters != NULL) {
		size_t length = strlen(shortOptionCharacters);
		for (int i = 0; i < length; i++) {
			if (shortOptionCharacters[i] == option) {
				*admitsArgument = i + 1 < length && shortOptionCharacters[i + 1] == ':';
				*isArgumentOptional = i + 2 < length && shortOptionCharacters[i + 2] == ':';
				return true;
			}
		}
	}

	return false;
}

static bool isShortOption(const char* argument) {
	size_t length = strlen(argument);

	return (length == 2 && strcmp("--", argument) == 0) ||
			(length >= 2 && argument[0] == '-' && !(argument[1] == '-' || argument[1] == '+' || argument[1] == '?' || argument[1] == ':'));
}

static bool longOptionExists(const char* optionName, const struct option* longOptions, bool* admitsArgument, bool* isArgumentOptional,
		char** embeddedArgumentForOption, const struct option** firstMatch, const struct option** secondMatch) {
	if (longOptions != NULL) {
		char* localEmbeddedArgumentForOption = NULL;
		const struct option* localFirstMatch = NULL;
		const struct option* localSecondMatch = NULL;

		int matchCount = 0;
		int i = 0;
		const struct option* option;
		while ((option = &longOptions[i++])->name != NULL) {
			size_t inputOptionLength;
			localEmbeddedArgumentForOption = strchr(optionName, '=');
			if (localEmbeddedArgumentForOption != NULL) {
				inputOptionLength = localEmbeddedArgumentForOption - optionName;
			} else {
				inputOptionLength = strlen(optionName);
			}

			size_t optionLength = strlen(option->name);

			if (inputOptionLength <= optionLength && strncmp(optionName, option->name, inputOptionLength) == 0) {
				if (matchCount == 0) {
					localFirstMatch = option;

				} else if (matchCount == 1) {
					localSecondMatch = option;
				}
				matchCount++;
			}
		}

		if (firstMatch != NULL) {
			*firstMatch = localFirstMatch;
		}
		if (secondMatch != NULL) {
			*secondMatch = localSecondMatch;
		}
		if (embeddedArgumentForOption != NULL) {
			*embeddedArgumentForOption = localEmbeddedArgumentForOption;
		}

		if (matchCount == 1) {
			*admitsArgument = localFirstMatch->has_arg == required_argument || localFirstMatch->has_arg == optional_argument;
			*isArgumentOptional = localFirstMatch->has_arg == optional_argument;
			return true;
		}
	}

	return false;
}

static const char* getArgumentForOption(int argc, const char** argv,
		int* nextArgumentIndex, int* nextArgumentCharacterIndex,
		bool admitsArgument, bool isArgumentOptional) {
	const char* result = NULL;
	if (*nextArgumentIndex < argc) {
		const char* argument = argv[*nextArgumentIndex];

		if (*nextArgumentCharacterIndex > 1) {
			result = argument + *nextArgumentCharacterIndex;
			(*nextArgumentIndex)++;
			*nextArgumentCharacterIndex = 1;

		} else {
			bool nextArgumentIsShortOption = isShortOption(argument);
			bool nextArgumentIsLongOption = isLongOption(argument);
			if (admitsArgument && ((!nextArgumentIsShortOption && !nextArgumentIsLongOption && isArgumentOptional) || !isArgumentOptional)) {
				result = argument;

				(*nextArgumentIndex)++;
				*nextArgumentCharacterIndex = 1;
			}
		}
	}
	return result;
}

static int permutateNonOptionArguments(int argc, const char** argv, struct CommandLineArgumentsParsingConfiguration* configuration) {
	int nextArgumentIndex = 1;
	int nextArgumentCharacterIndex = 1;
	int firstPermutatedArgumentIndex = argc;

	while (nextArgumentIndex < firstPermutatedArgumentIndex) {
		bool admitsArgument;
		bool isArgumentOptional;
		const char* argument = argv[nextArgumentIndex];

		if (isShortOption(argument)) {
			char option = argument[nextArgumentCharacterIndex];
			size_t argumentLength = strlen(argument);
			if (++nextArgumentCharacterIndex >= argumentLength) {
				nextArgumentIndex++;
				nextArgumentCharacterIndex = 1;
			}

			if (shortOptionExists(option, configuration->shortOptionCharacters, &admitsArgument, &isArgumentOptional)) {
				getArgumentForOption(argc, argv,
						&nextArgumentIndex, &nextArgumentCharacterIndex,
						admitsArgument, isArgumentOptional);
			}

		} else if (isLongOption(argument)) {
			nextArgumentIndex++;

			char* embeddedArgumentForOption;
			if (longOptionExists(argument + 2, configuration->longOptions, &admitsArgument, &isArgumentOptional, &embeddedArgumentForOption, NULL, NULL)) {
				if (embeddedArgumentForOption == NULL) {
					getArgumentForOption(argc, argv,
							&nextArgumentIndex, &nextArgumentCharacterIndex,
							admitsArgument, isArgumentOptional);
				}
			}

		} else {
			memmove(&argv[nextArgumentIndex], &argv[nextArgumentIndex + 1], (firstPermutatedArgumentIndex - nextArgumentIndex - 1) * sizeof(char*));
			firstPermutatedArgumentIndex--;
			argv[firstPermutatedArgumentIndex] = argument;
		}
	}

	for (int begin = firstPermutatedArgumentIndex, end = argc - 1; begin < end; begin++, end--) {
		const char* argument = argv[begin];
		argv[begin] = argv[end];
		argv[end] = argument;
	}

	return firstPermutatedArgumentIndex;
	return 0;
}

void commandLineUtilsInitializeContext(struct CommandLineArgumentsParsingContext* context, int nextArgumentIndex, int nextArgumentCharacterIndex) {
	context->nextArgumentIndex = nextArgumentIndex;
	context->nextArgumentCharacterIndex = nextArgumentCharacterIndex;
}

void commandLineUtilsParseArguments(int argc, const char** argv, struct CommandLineArgumentsParsingConfiguration* configuration,
		struct CommandLineArgumentsParsingContext* context, struct CommandLineArgumentsParsingResult* result) {
	int nextArgumentIndex = context->nextArgumentIndex;
	int nextArgumentCharacterIndex = context->nextArgumentCharacterIndex;

	memset(result, 0, sizeof(struct CommandLineArgumentsParsingResult));
	result->errorType = COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS;

	if (nextArgumentIndex < argc) {
		while (nextArgumentIndex < argc) {
			const char* argument = argv[nextArgumentIndex];
			bool admitsArgument;
			bool isArgumentOptional;

			result->argument = NULL;

			if (isShortOption(argument)) {
				size_t argumentLength = strlen(argument);

				if (strcmp("--", argument) == 0) {
					nextArgumentIndex++;

				} else {
					result->optionValue = argument[nextArgumentCharacterIndex];
					if (++nextArgumentCharacterIndex >= argumentLength) {
						nextArgumentIndex++;
						nextArgumentCharacterIndex = 1;
					}

					if (shortOptionExists(result->optionValue, configuration->shortOptionCharacters, &admitsArgument, &isArgumentOptional)) {
						if (admitsArgument) {
							const char* argumentForOption = getArgumentForOption(argc, argv,
									&nextArgumentIndex, &nextArgumentCharacterIndex,
									admitsArgument, isArgumentOptional);

							if (argumentForOption != NULL) {
								result->errorType = COMMAND_LINE_UTILS_NO_ERROR;
								result->argument = argumentForOption;

							} else if (isArgumentOptional) {
								result->errorType = COMMAND_LINE_UTILS_NO_ERROR;

							} else {
								result->errorType = COMMAND_LINE_UTILS_MISSING_REQUIRED_PARAMETER;
							}

						} else {
							result->errorType = COMMAND_LINE_UTILS_NO_ERROR;
						}

					} else {
						result->errorType = COMMAND_LINE_UTILS_UNKNOWN_OPTION;
					}
				}

				break;

			} else if (isLongOption(argument)) {
				nextArgumentIndex++;

				char* embeddedArgumentForOption = NULL;
				const struct option* firstMatch = NULL;
				const struct option* secondMatch = NULL;
				if (longOptionExists(argument + 2, configuration->longOptions, &admitsArgument, &isArgumentOptional,
						&embeddedArgumentForOption, &firstMatch, &secondMatch)) {
					assert(firstMatch != NULL);
					if (firstMatch->flag == NULL) {
						result->optionValue = firstMatch->val;
					} else {
						*firstMatch->flag = firstMatch->val;
						result->optionValue = 0;
					}

					if (admitsArgument) {
						if (embeddedArgumentForOption != NULL) {
							result->errorType = COMMAND_LINE_UTILS_NO_ERROR;
							result->argument = embeddedArgumentForOption + 1; /* As it needs to remove '=' character. */

						} else {
							const char* argumentForOption = getArgumentForOption(argc, argv,
									&nextArgumentIndex, &nextArgumentCharacterIndex,
									admitsArgument, isArgumentOptional);

							if (argumentForOption != NULL) {
								result->errorType = COMMAND_LINE_UTILS_NO_ERROR;
								result->argument = argumentForOption;

							} else if (isArgumentOptional) {
								result->errorType = COMMAND_LINE_UTILS_NO_ERROR;

							} else {
								result->errorType = COMMAND_LINE_UTILS_MISSING_REQUIRED_PARAMETER;
							}
						}

					} else {
						if (embeddedArgumentForOption != NULL) {
							result->errorType = COMMAND_LINE_UTILS_NO_PARAMETER_ALLOWED;
							result->argument = embeddedArgumentForOption + 1; /* As it needs to remove '=' character. */

						} else {
							result->errorType = COMMAND_LINE_UTILS_NO_ERROR;
						}
					}

				} else {
					assert((firstMatch == NULL && secondMatch == NULL) || (firstMatch != NULL && secondMatch != NULL));

					if (firstMatch != NULL) {
						result->errorType = COMMAND_LINE_UTILS_MORE_THAN_ONE_OPTION_WITH_PREFIX;
					} else {
						result->errorType = COMMAND_LINE_UTILS_UNKNOWN_OPTION;
					}
				}

				result->option1 = firstMatch;
				result->option2 = secondMatch;

				break;

			} else if (configuration->operationMode == HANDLE_NON_OPTIONS_AS_ARGUMENT_OF_ONE) {
				result->optionValue = 1;
				result->argument = argument;
				result->errorType = COMMAND_LINE_UTILS_NO_ERROR;
				nextArgumentIndex++;

				break;

			} else if (configuration->operationMode == STOP_ON_FIRST_NON_OPTION) {
				nextArgumentIndex--;

				result->optionValue = 0;
				result->argument = NULL;
				result->errorType = COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS;
				nextArgumentIndex++;

				break;

			} else {
				nextArgumentIndex++;
			}
		}
	}

	if (result->errorType == COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS && configuration->operationMode == MOVE_NON_OPTIONS_TO_END) {
		nextArgumentIndex = permutateNonOptionArguments(nextArgumentIndex, argv, configuration);
	}

	context->nextArgumentIndex = nextArgumentIndex;
	context->nextArgumentCharacterIndex = nextArgumentCharacterIndex;
}

void commandLineUtilsSplit(const char* rawCommandLine,
		char* argumentsBuffer, size_t argumentsBufferSize,
		const char** arguments, int argumentsLength,
		int* argumentCount) {

	int localArgumentCount = 0;

	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, argumentsBuffer, argumentsBufferSize);

	bool foundSingleQuote = false;
	bool foundDoubleQuote = false;
	const char* argument = NULL;
	int i = 0;
	while (localArgumentCount < argumentsLength && stringStreamWriterGetAvailable(&stringStreamWriter) > 0) {
		char c = rawCommandLine[i++];
		if (c == '\'' && !foundDoubleQuote) {
			if (foundSingleQuote) {
				if (argument != NULL) {
					streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '\0');
					arguments[localArgumentCount++] = argument;
					assert(strlen(argument) > 0);
					argument = NULL;
				}
				foundSingleQuote = false;

			} else {
				if (argument != NULL) {
					streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '\0');
					arguments[localArgumentCount++] = argument;
					assert(strlen(argument) > 0);
					argument = NULL;
				}
				foundSingleQuote = true;
			}

		} else if (c == '"' && !foundSingleQuote) {
			if (foundDoubleQuote) {
				if (argument != NULL) {
					streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '\0');
					arguments[localArgumentCount++] = argument;
					assert(strlen(argument) > 0);
					argument = NULL;
				}
				foundDoubleQuote = false;

			} else {
				if (argument != NULL) {
					streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '\0');
					arguments[localArgumentCount++] = argument;
					assert(strlen(argument) > 0);
					argument = NULL;
				}
				foundDoubleQuote = true;
			}

		} else if ((isspace(c) || c == '\0') && !foundSingleQuote && !foundDoubleQuote) {
			if (argument != NULL) {
				streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '\0');
				arguments[localArgumentCount++] = argument;
				assert(strlen(argument) > 0);
				argument = NULL;
			}

		} else {
			if (argument == NULL) {
				argument = stringStreamWriterNextCharacterPointer(&stringStreamWriter);
			}
			streamWriterWriteCharacter(&stringStreamWriter.streamWriter, c);
		}

		if (c == '\0') {
			break;
		}
	}

	*argumentCount = localArgumentCount;
}
