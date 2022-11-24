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

#ifndef FORMATTER_H
	#define FORMATTER_H

	#include <stdarg.h>
	#include <stdbool.h>
	#include <stdint.h>
	#include <time.h>

	#include <sys/types.h>

	#include <util/stream_writer.h>

	ssize_t formatterFormat(struct StreamWriter* streamWriter, const char* format, va_list ap, size_t* requiredLength);
	ssize_t formatterFormatDateTime(struct StreamWriter* streamWriter, const char* format, const struct tm* tmInstance,
		bool emmitNullCharacter, size_t* requiredLength);

#endif
