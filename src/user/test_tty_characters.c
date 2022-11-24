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

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
	printf("    0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
	for (int i = 0; i < 256; i++) {
		if (i % 16 == 0) {
			if (i > 0) {
				printf("\n");
			}
			printf("%3.X ", i / 16);
		}

		if (iscntrl(i)) {
			putchar('\x1B');
		}
		putchar(i);
		putchar(' ');
	}
	printf("\n");

	return EXIT_SUCCESS;
}
