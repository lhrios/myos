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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void bye(void) {
	printf("Bye!\n");
}

int main(int argc, char** argv) {
	printf("Hello world!\n");
	for (int i = 0; i < argc; i++) {
		printf("argv[%d] = \"%s\"\n", i, argv[i]);
	}

	if (argc >= 2) {
		int seconds = atoi(argv[1]);
		if (0 < seconds && seconds <= 60) {
			printf("I will sleep during %d seconds.\n", seconds);
			sleep(seconds);
			printf("Done!\n");
		}
	}

	atexit(&bye);

	return EXIT_SUCCESS;
}
