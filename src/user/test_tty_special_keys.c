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
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "user/util/tty_utils.h"

static struct termios termiosInitialInstance;

static void restoreTTY(void) {
	printf("\x1b[?1049h");
	printf("\x1b[2J");
	printf("\x1b[?1049l");
	tcsetattr(1, TCSANOW, &termiosInitialInstance);
}


static void initializeTTY(void) {
	setvbuf(stdout, NULL, _IONBF, 0);

	struct termios termiosInstance;
	tcgetattr(STDOUT_FILENO, &termiosInstance);
	memcpy(&termiosInitialInstance, &termiosInstance, sizeof(struct termios));
	termiosInstance.c_lflag &= (~ECHO & ~ICANON);
	tcsetattr(1, TCSANOW, &termiosInstance);

	atexit(restoreTTY);

	printf("\x1b[?1049h");
	printf("\x1b[2J");
	printf("\x1b[?25h");
}

int main(void) {
	initializeTTY();

	struct TTYUtilsKeyEscapeSequenceParsingContext context;
	ttyUtilsInitializeKeyEscapeSequenceParsingContext(&context);

	int originalFlags = 0;
 	originalFlags = fcntl(STDIN_FILENO, F_GETFL);
 	printf("%X", originalFlags);

	while (true) {
		const int bufferSize = 128;
		char buffer[bufferSize];

		int nextCharacter = 0;
		int availableCharacters = read(STDIN_FILENO, &buffer, sizeof(char) * bufferSize);

		while (availableCharacters > 0) {
			bool isLast = false;
			char character = buffer[nextCharacter++];
			availableCharacters--;
			if (availableCharacters == 0) {
				if (-1 == fcntl(STDIN_FILENO, F_SETFL, originalFlags | O_NONBLOCK)) {
					perror(NULL);
				}
				availableCharacters = read(STDIN_FILENO, &buffer, sizeof(char) * bufferSize);
				if (availableCharacters == -1) {
					assert(errno == EAGAIN);
					isLast = true;
				}
				if (-1 == fcntl(STDIN_FILENO, F_SETFL, originalFlags)) {
					perror(NULL);
				}
			}

			int key = ttyUtilsParseKeyEscapeSequence(&context, character, isLast);
			if (key != NO_KEY_AVAILABLE_YET) {
				switch(key) {
					case 'c':
					{
						int column;
						int row;

						ttyUtilsGetCursorPosition(&context, STDIN_FILENO, STDOUT_FILENO, &row, &column);
						printf("\x1B[s");
						printf("\x1B[1,1H");
						printf("\x1B[2K");
						printf("%d;%d", row, column);
						printf("\x1B[u");

					} break;

					case KEY_DOWN:
						printf("\x1b[B");
						break;

					case KEY_UP:
						printf("\x1b[A");
						break;

					case KEY_RIGHT:
						printf("\x1b[C");
						break;

					case KEY_LEFT:
						printf("\x1b[D");
						break;

					case KEY_ESCAPE:
					case 'q':
						goto END;
						break;
				}
			}
		}
	}
	END:;

	return EXIT_SUCCESS;
}
