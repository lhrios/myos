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

#ifndef TTY_UTILS_H
	#define TTY_UTILS_H

	#include <limits.h>
	#include <stdbool.h>
	#include <stddef.h>

	#define KEY_DOWN 0x1FF
	#define KEY_UP 0x2FF
	#define KEY_LEFT 0x3FF
	#define KEY_RIGHT 0x4FF

	#define KEY_ESCAPE 0x1B

	#define KEY_BACKSPACE 0x7F

	#define NO_KEY_AVAILABLE_YET LONG_MIN

	struct TTYUtilsKeyEscapeSequenceParsingContext {
		bool foundEscape;
		bool foundSquareOpenBracket;
		bool foundSemicolon;
		size_t rowAsStringLength;
		size_t columnAsStringLength;
	};

	void ttyUtilsInitializeKeyEscapeSequenceParsingContext(struct TTYUtilsKeyEscapeSequenceParsingContext* context);
	int ttyUtilsParseKeyEscapeSequence(struct TTYUtilsKeyEscapeSequenceParsingContext* context, char character, bool isLast);
	bool ttyUtilsGetCursorPosition(struct TTYUtilsKeyEscapeSequenceParsingContext* context,
			int inputFileDescriptorIndex, int outputFileDescriptorIndex, int* row, int* column);

#endif
