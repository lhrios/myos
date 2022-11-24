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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "util/double_linked_list.h"

enum SelectionMode {
	NO_SELECTION,
	BYTE_LEVEL_SELECTION,
	CHARACTER_LEVEL_SELECTION,
	FIELD_LEVEL_SELECTION
};

struct Range {
	 struct DoubleLinkedListElement listElement;
    int begin;
    int end;
};

enum SelectionMode selectionMode = NO_SELECTION;
static int delimiter = -1;
static const char* filePath = NULL;
static struct DoubleLinkedList rangeList;

static bool parseRanges(char* rangesAsString) {
	bool succeeded = true;
	char* context;
	for (int i = 0; succeeded; i++) {
		const char* rangeAsString = strtok_r(i == 0 ? rangesAsString : NULL, ",", &context);
		if (rangeAsString == NULL) {
			break;

		} else {
			struct Range* range = malloc(sizeof(struct Range));
			memset(range, 0, sizeof(struct Range));
			range->begin = -1;
			range->end = -1;

			const char* hifen = strchr(rangeAsString, '-');
			if (hifen == NULL) {
				char* startOfNotProcessedString;
				range->begin = strtol(rangeAsString, &startOfNotProcessedString, 10);
				range->end = range->begin;
				succeeded = startOfNotProcessedString[0] == '\0';

			} else {
				char* startOfNotProcessedString;
				if (rangeAsString != hifen) {
					range->begin = strtol(rangeAsString, &startOfNotProcessedString, 10);
					succeeded = startOfNotProcessedString[0] == '-';
				}

				if (*(hifen + 1) != '\0') {
					range->end = strtol(hifen + 1, &startOfNotProcessedString, 10);
					succeeded = succeeded && startOfNotProcessedString[0] == '\0';
				}

				succeeded = succeeded && ((range->begin == -1 && range->end > 0)
						|| (range->begin > 0 && range->end == -1)
						|| (range->begin > 0 && range->end > 0 && range->begin <= range->end));
			}

			doubleLinkedListInsertAfterLast(&rangeList, &range->listElement);
		}
	}

	free(rangesAsString);
	return succeeded;
}

static bool parseCommandLine(int argc, char** argv) {
	doubleLinkedListInitialize(&rangeList);

	bool succeeded = true;
	int result;
	while (succeeded && (result = getopt(argc, argv, "d:f:b:c:")) != -1) {
		switch (result) {
			case 'b':
			case 'c':
			case 'f':
				if (parseRanges(strdup(optarg))) {
					if (result == 'f') {
						selectionMode = FIELD_LEVEL_SELECTION;
					} else if (result == 'c') {
						selectionMode = CHARACTER_LEVEL_SELECTION;
					} else {
						selectionMode = BYTE_LEVEL_SELECTION;
					}
				} else {
					succeeded = false;
				}
				break;

			case 'd':
				if (strlen(optarg) == 1) {
					delimiter = optarg[0];
				}
				break;

			case '?':
				succeeded = false;
				break;

			case -1:
				break;
		}
	}

	if (argc - optind == 1) {
		filePath = argv[optind];
	} else if (argc - optind > 1) {
		succeeded = false;
	}

	succeeded = succeeded && doubleLinkedListSize(&rangeList) > 0 && selectionMode != NO_SELECTION
			&& (selectionMode == FIELD_LEVEL_SELECTION || delimiter == -1);

	if (selectionMode == FIELD_LEVEL_SELECTION && delimiter == -1) {
		delimiter = '\t';
	}

	return succeeded;
}

static bool mustPrintCharacter(int index) {
	struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(&rangeList);
	while (listElement != NULL) {
		struct Range* range = (void*) listElement;
		listElement = listElement->next;

		if ((range->begin == -1 || range->begin <= index) && (range->end == -1 || index <= range->end)) {
			return true;
		}
	}

	return false;
}

static bool printSelectedParts(void) {
	bool succeeded = true;
	FILE* stream;
	if (filePath == NULL) {
		stream = stdin;
	} else {
		stream = fopen(filePath, "r");
		if (stream == NULL) {
			succeeded = false;
			perror(NULL);
		}
	}

	if (succeeded) {
		int c;
		int byteIndex = 1;
		int fieldIndex = 1;
		int rowPrintCount = 0;
		int fieldLength = 0;

		while ((c = fgetc(stream)) != EOF) {
			if (selectionMode == FIELD_LEVEL_SELECTION) {
				if (c == delimiter) {
					fieldIndex++;
					fieldLength = 0;

				} else if (c == '\n') {
					putchar('\n');
					fieldIndex = 1;
					rowPrintCount = 0;
					fieldLength = 0;

				} else {
					if (mustPrintCharacter(fieldIndex)) {
						if (fieldLength == 0 && rowPrintCount > 0) {
							putchar(delimiter);
						}

						putchar(c);
						rowPrintCount++;
						fieldLength++;
					}
				}

			} else {
				if (c == '\n') {
					putchar('\n');
					byteIndex = 0;
				} else if (mustPrintCharacter(byteIndex)) {
					putchar(c);
				}
				byteIndex++;
			}
		}
	}

	return succeeded;
}

int main(int argc, char** argv) {
	int result = EXIT_FAILURE;
   if (parseCommandLine(argc, argv)) {
   	result = printSelectedParts() ? EXIT_SUCCESS : EXIT_FAILURE;

   } else {
   	 fprintf(stderr, "Invalid program evocation\n");
   }
	return result;
}
