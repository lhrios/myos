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

#include <limits.h>
#include <stdio.h>

#include "util/formatter.h"
#include "util/string_stream_writer.h"

int sprintf(char* buffer, const char* format, ...) {
	va_list ap;

	va_start(ap, format);
	int result = vsprintf(buffer, format, ap);
	va_end(ap);
	return result;
}

int vsprintf(char* buffer, const char *format, va_list ap) {
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, UINT_MAX);
	ssize_t result = formatterFormat(&stringStreamWriter.streamWriter, format, ap, NULL);
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	return result;
}

int snprintf(char* buffer, size_t size, const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	int result = vsnprintf(buffer, size, format, ap);
	va_end(ap);
	return result;
}

int vsnprintf(char* buffer, size_t size, const char *format, va_list ap) {
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, size);
	size_t requiredLength;
	formatterFormat(&stringStreamWriter.streamWriter, format, ap, &requiredLength);
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	return requiredLength;
}
