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

#ifndef KERNEL_VGA_H
	#define KERNEL_VGA_H

	#include <stdbool.h>
	#include <stdint.h>

	#include "kernel/multiboot.h"
	#include "kernel/vga_colors.h"

	#include "util/string_stream_writer.h"

	#define COMBINE_COLORS(foregroundColor, backgroundColor) ((uint8_t) ((backgroundColor << 4) | foregroundColor))

	void vgaInitialize(struct multiboot_info *multiboot_info, struct StringStreamWriter* stringStreamWriter);

	int vgaGetColumnCount(void);
	int vgaGetRowCount(void);
	uint16_t* vgaGetFrameBuffer(void);

	bool vgaIsCursorEnabled(void);
	void vgaSetCursor(uint16_t cursorColumn, uint16_t cursorRow);
	void vgaDisableCursor(void);
	void vgaEnableCursor(void);
#endif
