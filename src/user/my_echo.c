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

#include <ctype.h>
#include <stdbool.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static void doEchoInterpretatingBackslashEscapes(const char* argument) {
	bool foundBackslash = false;
	bool isOctalValue = false;
	bool isHexadecimalValue = false;

	char digitsBuffer[4];
	int digitsCount = 0;

	while (true) {
		char c = *argument++;
		if (foundBackslash) {
			switch (c) {
				case '0':
					isOctalValue = true;
					break;

				case 'x':
					isHexadecimalValue = true;
					break;

				case '\\':
					printf("%c", '\\');
					foundBackslash = false;
					break;

				case 'n':
					printf("%c", '\n');
					foundBackslash = false;
					break;

				default: {
					bool characterConsumed = false;
					if ((isdigit(c) && isOctalValue) || (isxdigit(c) && isHexadecimalValue)) {
						digitsBuffer[digitsCount++] = c;
						characterConsumed = true;
					}

					if ((digitsCount == 2 || !isxdigit(c)) && isHexadecimalValue) {
						digitsBuffer[digitsCount++] = '\0';
						printf("%c", (int) strtol(digitsBuffer, NULL, 16));
						isHexadecimalValue = false;
						foundBackslash = false;
						digitsCount = 0;

					} else if ((digitsCount == 3 || !isdigit(c)) && isOctalValue) {
						digitsBuffer[digitsCount++] = '\0';
						printf("%c", (int) strtol(digitsBuffer, NULL, 8));
						isOctalValue = false;
						foundBackslash = false;
						digitsCount = 0;
					}

					if (!characterConsumed) {
						printf("%c", c);
					}
				} break;
			}

		} else if (c == '\\') {
			foundBackslash = true;

		} else if (c == '\0') {
			break;

		} else {
			printf("%c", c);
		}
	}
}

int main(int argc, char** argv) {
	bool outputTrailingNewline = true;
	bool interpretateBackslashEscapes = false;

	int result;
	while ((result = getopt(argc, argv, "+:eEn")) != -1) {
		switch (result) {
			case 'e':
				interpretateBackslashEscapes = true;
				break;

			case 'E':
				interpretateBackslashEscapes = false;
				break;

			case 'n':
				outputTrailingNewline = false;
				break;
		}
	}

	bool first = true;
	for (int i = optind; i < argc; i++) {
		if (first) {
			first = false;
		} else {
			printf(" ");
		}
		if (interpretateBackslashEscapes) {
			doEchoInterpretatingBackslashEscapes(argv[i]);
		} else {
			printf("%s", argv[i]);
		}
	}
	if (outputTrailingNewline) {
		printf("\n");
	}

	return EXIT_SUCCESS;
}
