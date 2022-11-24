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
#include <unistd.h>

int main(int argc, char** argv) {
	opterr = 1;

	int result;
	const char* optstring = "+ab::c:";
	while ((result = getopt(argc, argv, optstring)) != -1) {
		printf("%c (%d):\n", result, result);
		if (result == '?') {
			printf("  optopt: %c\n", optopt);
		}
		printf("  optind: %d\n", optind - 1);
		printf("  optarg: %s\n", optarg);

		for (int i = 0; i < argc; i++) {
			printf("  argv[%d]: %s\n", i, argv[i]);
		}

		printf("\n");
	}

	printf("optind: %d\n", optind);
	for (int i = 0; i < argc; i++) {
		printf("argv[%d]: %s\n", i, argv[i]);
	}

	return EXIT_SUCCESS;
}
