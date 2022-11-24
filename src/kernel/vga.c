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

#include <stdlib.h>

#include "kernel/busy_waiting_manager.h"
#include "kernel/log.h"
#include "kernel/vbe.h"
#include "kernel/vga.h"
#include "kernel/x86.h"

#include "util/ring_buffer.h"

/*
 * References:
 * - Intel® OpenSource HD Graphics Programmer’s Reference Manual (PRM) Volume 3 Part 1: Display Registers –
 *	 VGA Registers (SandyBridge)
 * - VESA BIOS Extension (VBE) Core Functions Standard Version: 3.0
 * - XGA Software Programmer's Guide
 * - Hardware Level VGA and SVGA Video Programming Information Page - Free VGA Project
 */

static uint16_t columnCount;
static uint16_t rowCount;

static uint16_t* frameBuffer;

static bool isCursorEnabled;

bool vgaIsCursorEnabled(void) {
	return isCursorEnabled;
}

void vgaDisableCursor(void) {
	if (isCursorEnabled) {
		x86OutputByteToPort(0x3D4, 0x0A);
		x86OutputByteToPort(0x3D5, 0x20);

		isCursorEnabled = false;
	}
}

void vgaEnableCursor(void) {
	if (!isCursorEnabled) {
		x86OutputByteToPort(0x3D4, 0x0A);
		x86OutputByteToPort(0x3D5, (x86InputByteFromPort(0x3D5) & 0xC0) | 0x0D);

		x86OutputByteToPort(0x3D4, 0x0B);
		x86OutputByteToPort(0x3D5, (x86InputByteFromPort(0x3D5) & 0xE0) | 0x0F);

		isCursorEnabled = true;
	}
}

void vgaSetCursor(uint16_t cursorColumn, uint16_t cursorRow) {
	assert(0 <= cursorColumn && cursorColumn < columnCount);
	assert(0 <= cursorRow && cursorRow < rowCount);

	if (isCursorEnabled) {
		uint16_t cursorLocation = (cursorRow * columnCount) + cursorColumn;
		x86OutputByteToPort(0x3D4, 0x0E);
		x86OutputByteToPort(0x3D5, cursorLocation >> 8);
		x86OutputByteToPort(0x3D4, 0x0F);
		x86OutputByteToPort(0x3D5, 0xFF & cursorLocation);
	}
}

static void* readFrameBufferAddress(void) {
	x86InputByteFromPort(0x03DA);
	x86OutputByteToPort(0x3CE, 0x06);
	switch ((x86InputByteFromPort(0x3CF) & 0x0C) >> 2) {
		case 0:
			return (void*) 0xA0000;

		case 1:
			return (void*) 0xA0000;

		case 2:
			return (void*) 0xB0000;

		case 3:
			return (void*) 0xB8000;

		default:
			x86Ring0Stop();
			return NULL;
	}
}

static uint32_t readColumnCount(void) {
	x86OutputByteToPort(0x03D4, 0x1);
	uint8_t horizontalDisplayEnableEnd = x86InputByteFromPort(0x03D4 + 1);
	return horizontalDisplayEnableEnd + 1;
}

static uint32_t readRowCount(void) {
	x86OutputByteToPort(0x03D4, 0x09);
	uint8_t startingRowScanCount = x86InputByteFromPort(0x03D4 + 1) & 0x1F;

	x86OutputByteToPort(0x03D4, 0x07);
	uint8_t overflowRegister = x86InputByteFromPort(0x03D4 + 1);

	x86OutputByteToPort(0x03D4, 0x12);
	uint32_t verticalDisplayEnableEnd = x86InputByteFromPort(0x03D4 + 1);
	verticalDisplayEnableEnd |= (((overflowRegister & 0x40) ? 1 : 0) << 9)
		| (((overflowRegister & 0x2) ? 1 : 0) << 8);

	return (verticalDisplayEnableEnd + 1) / (startingRowScanCount + 1);
}

static void disableBlink(void) {
	x86InputByteFromPort(0x03DA);
	/*
	 * "Palette Address Source": It is set to 1 for normal operation of the attribute controller.
	 * "Mode Control Register".
	 */
	x86OutputByteToPort(0x3C0, 0x20 | 0x10);
	x86OutputByteToPort(0x3C0, x86InputByteFromPort(0x3C1) & 0xF7);
}

static bool isOnTextMode(void) {
	bool onTextMode = true;

	x86InputByteFromPort(0x03DA);
	x86OutputByteToPort(0x3CE, 0x06);
	onTextMode = onTextMode && (x86InputByteFromPort(0x3CF) & 0x1) == 0;

	x86InputByteFromPort(0x03DA);
	x86OutputByteToPort(0x3C0, 0x20 | 0x10);
	onTextMode = onTextMode && (x86InputByteFromPort(0x3C1) & 0x1) == 0;

	return onTextMode;
}

static void printAllColors(struct StringStreamWriter* stringStreamWriter) {
	streamWriterFormat(&stringStreamWriter->streamWriter, "Printing all available colors to test the display:\n");

	/* Foreground colors: */
	streamWriterFormat(&stringStreamWriter->streamWriter, "  Foreground: ");
	for (int color = 0; color <= 0xF; color++) {
		streamWriterFormat(&stringStreamWriter->streamWriter, "\x1B[40;%dm\xDB\xDB\x1B[m", color + (color >= 0x8 ? 90 - 0x8 : 30));
	}
	streamWriterFormat(&stringStreamWriter->streamWriter, "\n");

	/* Background colors: */
	streamWriterFormat(&stringStreamWriter->streamWriter, "  Background: ");
	for (int color = 0; color <= 0xF; color++) {
		streamWriterFormat(&stringStreamWriter->streamWriter, "\x1B[30;%dm  \x1B[m", color + (color >= 0x8 ? 100 - 0x8 : 40));
	}
	streamWriterFormat(&stringStreamWriter->streamWriter, "\n");
}

// TODO: Use available mode
static void handleGraphicalMode(uint8_t* originalFrameBuffer, uint32_t width, uint32_t height, uint32_t bytesPerPixel) {
	uint32_t colorIndex = 0;
	uint8_t colors[] = {0x1F, 0x7F, 0xFF};

	while (true) {
		uint8_t* frameBuffer = originalFrameBuffer;
		uint8_t color = colors[colorIndex++ % (sizeof(colors) / sizeof(uint8_t))];

		for (int i = 0; i < height * width; i++) {
			if (bytesPerPixel == 8) {
				*frameBuffer++ = color;

			} else if (bytesPerPixel == 24) {
				*frameBuffer++ = color;
				*frameBuffer++ = 0x00;
				*frameBuffer++ = 0x00;

			} else if (bytesPerPixel == 8) {
				*frameBuffer++ = color;
				*frameBuffer++ = 0x00;
				*frameBuffer++ = 0x00;
				*frameBuffer++ = 0x00;
			}
		}

		busyWaitingSleep(250);
	}
}

void vgaInitialize(struct multiboot_info *multiboot_info, struct StringStreamWriter* stringStreamWriter) {
	if (multiboot_info != NULL && (multiboot_info->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)) {
		if (multiboot_info->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT
				&& multiboot_info->framebuffer_addr < 0xFFFFFFFFLL) {
			rowCount = multiboot_info->framebuffer_height;
			columnCount = multiboot_info->framebuffer_width;
			frameBuffer = (void*) ((uint32_t) multiboot_info->framebuffer_addr);

			isCursorEnabled = false;
			vgaEnableCursor();

			streamWriterFormat(&stringStreamWriter->streamWriter, "Multiboot framebuffer table:\n"
				"  framebuffer_height=%d\n"
				"  framebuffer_width=%d\n"
				"  framebuffer_addr=%p\n",

				multiboot_info->framebuffer_height,
				multiboot_info->framebuffer_width,
				multiboot_info->framebuffer_addr);

		} else {
			if (multiboot_info->framebuffer_addr < 0xFFFFFFFFLL) {
				handleGraphicalMode((void*) (uint32_t) multiboot_info->framebuffer_addr, multiboot_info->framebuffer_width,
						multiboot_info->framebuffer_height, multiboot_info->framebuffer_bpp);
			}

			x86Ring0Stop();
		}

	} else if (multiboot_info != NULL && (multiboot_info->flags & MULTIBOOT_INFO_VBE_INFO)) {
		struct VbeInfoBlock *vbeInfoBlock = (void*) multiboot_info->vbe_control_info;
		struct ModeInfoBlock *modeInfoBlock = (void*) multiboot_info->vbe_mode_info;

		/* Is text mode the current mode? */
		if ((modeInfoBlock->ModeAttributes & 0x10) == 0) {
			rowCount = modeInfoBlock->YResolution;
			columnCount = modeInfoBlock->XResolution;
			frameBuffer = readFrameBufferAddress();

			isCursorEnabled = false;
			vgaEnableCursor();

			/*
			* It is important to note that it can not access the string pointers fields.
			* As they have been obtained during real mode, they require the segment information to be resolved properly.
			* However, we just have the segment selector here.
			*/

			streamWriterFormat(&stringStreamWriter->streamWriter, "Multiboot VBE table:\n"
				"  vbe_mode=%X\n"
				"    vbe_control_info:\n"
				"      VESASignature=%c%c%c%c\n"
				"      VESAVersion=%X\n"
				"    vbe_mode_info:\n"
				"      XResolution=%d\n"
				"      YResolution=%d\n"
				"      MemoryModel=%X\n",

				multiboot_info->vbe_mode,
				vbeInfoBlock->VESASignature[0],
				vbeInfoBlock->VESASignature[1],
				vbeInfoBlock->VESASignature[2],
				vbeInfoBlock->VESASignature[3],
				vbeInfoBlock->VESAVersion,
				modeInfoBlock->XResolution,
				modeInfoBlock->YResolution,
				modeInfoBlock->MemoryModel);

		} else {
			handleGraphicalMode(modeInfoBlock->PhysBasePtr, modeInfoBlock->XResolution, modeInfoBlock->YResolution, modeInfoBlock->BitsPerPixel);

			x86Ring0Stop();
		}

	} else {
		/*
		 * The boot loader has no information for us. We will do our best to probe the hardware directly.
		 * We try to avoid this as much as possible as the VGA documentation is not very clear/easy.
		 */

		if (isOnTextMode()) {
			frameBuffer = readFrameBufferAddress();
			columnCount = readColumnCount();
			rowCount = readRowCount();

			isCursorEnabled = false;
			vgaEnableCursor();

			streamWriterFormat(&stringStreamWriter->streamWriter, "VGA hardware probing result:\n"
				"  rowCount=%d\n"
				"  columnCount=%d\n"
				"  frameBuffer=%p\n",

				rowCount,
				columnCount,
				frameBuffer);

		} else {
			x86Ring0Stop();
		}
	}

	streamWriterFormat(&stringStreamWriter->streamWriter, "VGA hardware frameBuffer=%p\n", frameBuffer);

	disableBlink();
	printAllColors(stringStreamWriter);
}

int vgaGetColumnCount(void) {
	return columnCount;
}

int vgaGetRowCount(void) {
	return rowCount;
}

uint16_t* vgaGetFrameBuffer(void) {
	return frameBuffer;
}
