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

#include "user/util/checksum.h"

uint16_t fletcher16(const void* buffer, size_t count) {
	const uint8_t* castedBuffer = buffer;

	uint16_t sum1 = 0;
	uint16_t sum2 = 0;

	for(uint32_t i = 0; i < count; i++) {
		sum1 = (sum1 + castedBuffer[i]) % UCHAR_MAX;
		sum2 = (sum2 + sum1) % UCHAR_MAX;
	}

	return (sum2 << 8) | sum1;
}
