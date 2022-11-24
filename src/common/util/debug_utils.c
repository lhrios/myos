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

#include <stdbool.h>
#include <stdint.h>

#include "util/debug_utils.h"

void debugUtilsBuildHexadecimalTable(struct StreamWriter* streamWriter, void* buffer, size_t bufferSize) {
	const int NUMBER_OF_COLUMNS = 16;

	/* Header */
	streamWriterFormat(streamWriter, "\n");
	bool first = true;
	streamWriterFormat(streamWriter, "    |");
	for (int i = 0; i < NUMBER_OF_COLUMNS; i++) {
		if (first) {
			first = false;
		} else {
			streamWriterFormat(streamWriter, "|");
		}
		streamWriterFormat(streamWriter, "%.2X", i);
	}
	streamWriterFormat(streamWriter, "|\n");

	streamWriterFormat(streamWriter, "    +");
	for (int i = 0; i < NUMBER_OF_COLUMNS; i++) {
		streamWriterFormat(streamWriter, "--+");
	}

	/* Body */
	first = true;
	for (int i = 0; i < bufferSize; i++) {
		if (i % NUMBER_OF_COLUMNS == 0) {
			if (first) {
				first = false;
			} else {
				streamWriterFormat(streamWriter, "|");
			}
			streamWriterFormat(streamWriter, "\n%.2X  ", i / NUMBER_OF_COLUMNS);
		}
		streamWriterFormat(streamWriter, "|%.2X", ((uint8_t*) buffer)[i]);
	}
	streamWriterFormat(streamWriter, "|\n");
}
