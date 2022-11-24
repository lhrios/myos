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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "Invalid program evocation\n");
		return EXIT_FAILURE;
	}

	FILE* file1 = fopen(argv[1], "r");
	if (file1 == NULL) {
		perror(NULL);
	}

	FILE* file2 = fopen(argv[2], "r");
	if (file2 == NULL) {
		perror(NULL);
	}

	unsigned int charCount = 1;
	unsigned int lineCount = 1;

	while (true) {
		int char1 = fgetc(file1);
		int char2 = fgetc(file2);

		if (char1 != char2) {
			printf("%s and %s are not equal: byte %u, line %u\n", argv[1], argv[2], charCount, lineCount);
			return EXIT_FAILURE;

		} else if (char1 == EOF) {
			break;
		}

		charCount++;
		if (char1 == '\n') {
			lineCount++;
		}
	}

	return EXIT_SUCCESS;
}
