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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "util/scanner.h"

#include "user/util/tty_utils.h"

bool ttyUtilsGetCursorPosition(struct TTYUtilsKeyEscapeSequenceParsingContext* context,
		int inputFileDescriptorIndex, int outputFileDescriptorIndex, int* row, int* column) {
	write(outputFileDescriptorIndex, "\x1B[6n", strlen("\x1B[6n"));
	ttyUtilsInitializeKeyEscapeSequenceParsingContext(context);

	char rowAsString[32];
	char columnAsString[32];

	while (true) {
		char character;
		int result = read(inputFileDescriptorIndex, &character, sizeof(char));

		if (result != 1) {
			return false;

		} else if (character == '\x1B') {
			ttyUtilsInitializeKeyEscapeSequenceParsingContext(context);
			context->foundEscape = true;

		} else if (character == '[') {
			if (context->foundEscape && !context->foundSquareOpenBracket) {
				context->foundSquareOpenBracket = true;

			} else {
				ttyUtilsInitializeKeyEscapeSequenceParsingContext(context);
			}

		} else if (isdigit(character)) {
			if (context->foundEscape && context->foundSquareOpenBracket) {
				if (context->foundSemicolon) {
					if (context->columnAsStringLength + 1 + 1 <= 32) {
						columnAsString[context->columnAsStringLength++] = character;
					} else {
						ttyUtilsInitializeKeyEscapeSequenceParsingContext(context);
					}

				} else {
					if (context->rowAsStringLength + 1 + 1 <= 32) {
						rowAsString[context->rowAsStringLength++] = character;
					} else {
						ttyUtilsInitializeKeyEscapeSequenceParsingContext(context);
					}
				}

			} else {
				ttyUtilsInitializeKeyEscapeSequenceParsingContext(context);
			}

		} else if (character == ';') {
			if (context->foundEscape && context->foundSquareOpenBracket && !context->foundSemicolon) {
				context->foundSemicolon = true;
			} else {
				ttyUtilsInitializeKeyEscapeSequenceParsingContext(context);
			}

		} else if (character == 'R') {
			columnAsString[context->columnAsStringLength] = '\0';
			rowAsString[context->rowAsStringLength] = '\0';

			if (scannerParseInt32(columnAsString, 10, false, false, NULL, column) == 0 && scannerParseInt32(rowAsString, 10, false, false, NULL, row) == 0) {
				context->foundEscape = false;
				context->foundSquareOpenBracket = false;
				return true;

			} else {
				ttyUtilsInitializeKeyEscapeSequenceParsingContext(context);
			}

		} else {
			ttyUtilsInitializeKeyEscapeSequenceParsingContext(context);
		}
	}

	return false;
}

void ttyUtilsInitializeKeyEscapeSequenceParsingContext(struct TTYUtilsKeyEscapeSequenceParsingContext* context) {
	memset(context, 0, sizeof(struct TTYUtilsKeyEscapeSequenceParsingContext));
}

int ttyUtilsParseKeyEscapeSequence(struct TTYUtilsKeyEscapeSequenceParsingContext* context, char character, bool isLast) {
	if (character == '\x1B') {
		if (isLast) {
			return KEY_ESCAPE;

		} else {
			context->foundEscape = true;
			context->foundSquareOpenBracket = false;
		}

	} else {
		if (context->foundEscape) {
			if (character == '[') {
				context->foundSquareOpenBracket = true;

			} else if (context->foundSquareOpenBracket) {
				context->foundEscape = false;
				context->foundSquareOpenBracket = false;

				switch (character) {
					case 'A':
						return KEY_UP;

					case 'B':
						return KEY_DOWN;

					case 'C':
						return KEY_RIGHT;

					case 'D':
						return KEY_LEFT;
				}
			}

		} else {
			return character;
		}
	}

	return NO_KEY_AVAILABLE_YET;
}
