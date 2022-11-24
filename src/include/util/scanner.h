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

#ifndef SCANNER_H
	#define SCANNER_H

	#include <stdarg.h>
	#include <stdbool.h>
	#include <stddef.h>
	#include <stdint.h>

	#include "util/stream_reader.h"

	bool scannerIsBaseCharacter(int base, char character);
	int scannerGetCharacterValue(int base, char character);

	int scannerParseInt32(const char* string, int base, bool allowSpaceBefore, bool allowSign, const char** startOfNotProcessedString, int32_t* output);
	int scannerParseUint32(const char* string, int base, bool allowSpaceBefore, const char** startOfNotProcessedString, uint32_t* output);

	union ScannerInteger32 {
		int32_t signedValue;
		uint32_t unsignedValue;
	};

	int scannerParseInteger32(struct StreamReader* streamReader, int base, bool isSigned, bool allowSpaceBefore, bool allowSign, const char** startOfNotProcessedString, union ScannerInteger32* integer32);

#endif
