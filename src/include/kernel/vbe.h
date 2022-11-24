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

#ifndef VBE_HEADER
	#define VBE_HEADER 1

	/* SuperVGA information block */
	struct VbeInfoBlock {
		char VESASignature[4]; /* 'VESA' 4 byte signature */
		short VESAVersion; /* VBE version number */
		char* OEMStringPtr; /* Pointer to OEM string */
		long Capabilities; /* Capabilities of video card */
		void* VideoModePtr; /* Pointer to supported modes */
		short TotalMemory; /* Number of 64kb memory blocks */

		short OemSoftwareRev;
		char* OemVendorNamePtr;

		char reserved[236]; /* Pad to 256 byte block size */
	} __attribute__((packed));

	struct ModeInfoBlock {
		unsigned short ModeAttributes; /* Mode attributes */
		unsigned char WinAAttributes; /* Window A attributes */
		unsigned char WinBAttributes; /* Window B attributes */
		unsigned short WinGranularity; /* Window granularity in k */
		unsigned short WinSize; /* Window size in k */
		unsigned short WinASegment; /* Window A segment */
		unsigned short WinBSegment; /* Window B segment */
		void (*WinFuncPtr)(void); /* Pointer to window function */
		unsigned short BytesPerScanLine; /* Bytes per scanline */
		unsigned short XResolution; /* Horizontal resolution */
		unsigned short YResolution; /* Vertical resolution */
		unsigned char XCharSize; /* Character cell width */
		unsigned char YCharSize; /* Character cell height */
		unsigned char NumberOfPlanes; /* Number of memory planes */
		unsigned char BitsPerPixel; /* Bits per pixel */
		unsigned char NumberOfBanks; /* Number of CGA style banks */
		unsigned char MemoryModel; /* Memory model type */
		unsigned char BankSize; /* Size of CGA style banks */
		unsigned char NumberOfImagePages; /* Number of images pages */
		unsigned char res1; /* Reserved */
		unsigned char RedMaskSize; /* Size of direct color red mask */
		unsigned char RedFieldPosition; /* Bit posn of lsb of red mask */
		unsigned char GreenMaskSize; /* Size of direct color green mask */
		unsigned char GreenFieldPosition; /* Bit posn of lsb of green mask */
		unsigned char BlueMaskSize; /* Size of direct color blue mask */
		unsigned char BlueFieldPosition; /* Bit posn of lsb of blue mask */
		unsigned char RsvdMaskSize; /* Size of direct color res mask */
		unsigned char RsvdFieldPosition; /* Bit posn of lsb of res mask */
		unsigned char DirectColorModeInfo; /* Direct color mode attributes */

		void* PhysBasePtr;

		unsigned char res2[216]; /* Pad to 256 byte block size */
	} __attribute__((packed));

#endif
