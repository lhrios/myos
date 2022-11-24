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

#ifndef USER_SCANNER_H
	#define USER_SCANNER_H

	#include <stdarg.h>
	#include <stdbool.h>
	#include <stddef.h>
	#include <stdint.h>

	#include "util/scanner.h"

	int scannerParseInt32(const char* string, int base, bool allowSpaceBefore, bool allowSign, const char** startOfNotProcessedString, int32_t* output);
	int scannerParseUint32(const char* string, int base, bool allowSpaceBefore, const char** startOfNotProcessedString, uint32_t* output);

	int scannerParseFloat(const char* string, bool allowSpaceBefore, const char** startOfNotProcessedString, long double* output);

	int scannerScanFormat(struct StreamReader* streamReader, const char *format, va_list ap,
			void* memoryAllocatorContext, void (*memoryAllocatorRelease)(void*, void*), void* (*memoryAllocatorResize)(void*, void*, size_t),
			int* errorId);
#endif
