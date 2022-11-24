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

#ifndef KERNEL_VGA_COLORS_H
	#define KERNEL_VGA_COLORS_H

	enum VGAColorEnum {
		VGA_BLACK = 0x0,
		VGA_BLUE = 0x1,
		VGA_GREEN = 0x2,
		VGA_CYAN = 0x3,
		VGA_RED = 0x4,
		VGA_MAGENTA = 0x5,
		VGA_YELLOW = 0x6,
		VGA_WHITE = 0x7,

		VGA_BRIGHT_BLACK = 0x8,
		VGA_BRIGHT_BLUE = 0x9,
		VGA_BRIGHT_GREEN = 0xA,
		VGA_BRIGHT_CYAN = 0xB,
		VGA_BRIGHT_RED = 0xC,
		VGA_BRIGHT_MAGENTA = 0xD,
		VGA_BRIGHT_YELLOW = 0xE,
		VGA_BRIGHT_WHITE = 0xF
	};

#endif
