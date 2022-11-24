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

#include <termios.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
	printf("Testing raw input mode:\n");

	struct termios termiosInstance;
	if (-1 != tcgetattr(STDOUT_FILENO, &termiosInstance)) {
		termiosInstance.c_lflag &= ~ICANON;
		//termiosInstance.c_lflag &= ~ECHO;
		termiosInstance.c_lflag &= ~ECHOCTL;
		if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &termiosInstance)) {
			perror(NULL);
		}

	} else {
		perror(NULL);
	}

	while (1) {
		char buffer[512];
		ssize_t result = read(STDIN_FILENO, buffer, 512);
		for (int i = 0; i < result; i++) {
			printf("\n%d", buffer[i++]);
			fflush(stdout);
		}
	}

	return EXIT_SUCCESS;
}
