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

#include <kernel/key_event.h>
#include <kernel/log.h>

uint32_t KEYBOARD_F_KEYS_SCAN_CODES[] = {
	F1_KEY_SCAN_CODE,
	F2_KEY_SCAN_CODE,
	F3_KEY_SCAN_CODE,
	F4_KEY_SCAN_CODE,

	F5_KEY_SCAN_CODE,
	F6_KEY_SCAN_CODE,
	F7_KEY_SCAN_CODE,
	F8_KEY_SCAN_CODE,

	F9_KEY_SCAN_CODE,
	F10_KEY_SCAN_CODE,
	F11_KEY_SCAN_CODE,
	F12_KEY_SCAN_CODE
};

static int getASCIICharacter(bool isShiftKeyPressed, bool altGraphKeyPressed,
		int plainValue, int shiftValue, int altGraphValue, int shiftAndAltGraphValue) {

	if (!isShiftKeyPressed && !altGraphKeyPressed) {
		return plainValue;

	} else if (isShiftKeyPressed && !altGraphKeyPressed) {
		return shiftValue;

	} else if (!isShiftKeyPressed && altGraphKeyPressed) {
		return altGraphValue;

	} else {
		return shiftAndAltGraphValue;
	}
}

bool keyEventGetASCIICharacter(KeyEvent keyEvent, char* character){
	bool isShiftKeyPressed = (keyEvent & KEY_EVENT_SHIFT_KEY_PRESSED_MASK) != 0;
	bool altGraphKeyPressed = (keyEvent & KEY_EVENT_ALT_GRAPH_KEY_PRESSED_MASK) != 0;

	int localCharacter = -1;

	switch (KEY_EVENT_SCAN_CODE_MASK & keyEvent) {
		case 0x29:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, '\'', '"', 170, -1);
			break;
		case 0x02:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, '1', '!', -1, 173);
			break;
		case 0x03:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, '2', '@', 253, 171);
			break;
		case 0x04:
			localCharacter = isShiftKeyPressed ? '#' : '3';
			break;
		case 0x05:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, '4', '$', 156, 172);
			break;
		case 0x06:
			localCharacter = isShiftKeyPressed ? '%' : '5';
			break;
		case 0x07:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, '6', '"', 170, -1);
			break;
		case 0x08:
			localCharacter = isShiftKeyPressed ? '&' : '7';
			break;
		case 0x09:
			localCharacter = isShiftKeyPressed ? '*' : '8';
			break;
		case 0x0A:
			localCharacter = isShiftKeyPressed ? '(' : '9';
			break;
		case 0x0B:
			localCharacter = isShiftKeyPressed ? ')' : '0';
			break;
		case 0x0C:
			localCharacter = isShiftKeyPressed ? '_' : '-';
			break;
		case 0x0D:
			localCharacter = isShiftKeyPressed ? '+' : '=';
			break;

		case 0x0F:
			localCharacter = '\t';
			break;

		case 0x10:
			localCharacter = isShiftKeyPressed ? 'Q' : 'q';
			break;
		case 0x11:
			localCharacter = isShiftKeyPressed ? 'W' : 'w';
			break;
		case 0x12:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, 'e', 'E', -1, -1);
			break;
		case 0x13:
			localCharacter = isShiftKeyPressed ? 'R' : 'r';
			break;
		case 0x14:
			localCharacter = isShiftKeyPressed ? 'T' : 't';
			break;
		case 0x15:
			localCharacter = isShiftKeyPressed ? 'Y' : 'y';
			break;
		case 0x16:
			localCharacter = isShiftKeyPressed ? 'U' : 'u';
			break;
		case 0x17:
			localCharacter = isShiftKeyPressed ? 'I' : 'i';
			break;
		case 0x18:
			localCharacter = isShiftKeyPressed ? 'O' : 'o';
			break;
		case 0x19:
			localCharacter = isShiftKeyPressed ? 'P' : 'p';
			break;
		case 0x1A:
			localCharacter = isShiftKeyPressed ? '`' : '\'';
			break;
		case 0x1B:
			localCharacter = isShiftKeyPressed ? '{' : '[';
			break;

		case 0x1E:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, 'a', 'A', 145, 146);
			break;
		case 0x1F:
			localCharacter = isShiftKeyPressed ? 'S' : 's';
			break;
		case 0x20:
			localCharacter = isShiftKeyPressed ? 'D' : 'd';
			break;
		case 0x21:
			localCharacter = isShiftKeyPressed ? 'F' : 'f';
			break;
		case 0x22:
			localCharacter = isShiftKeyPressed ? 'G' : 'g';
			break;
		case 0x23:
			localCharacter = isShiftKeyPressed ? 'H' : 'h';
			break;
		case 0x24:
			localCharacter = isShiftKeyPressed ? 'J' : 'j';
			break;
		case 0x25:
			localCharacter = isShiftKeyPressed ? 'K' : 'k';
			break;
		case 0x26:
			localCharacter = isShiftKeyPressed ? 'L' : 'l';
			break;
		case 0x28:
			localCharacter = isShiftKeyPressed ? '^' : '~';
			break;
		case 0x2B:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, ']', '}', 167, 167);
			break;

		case 0x56:
			localCharacter = isShiftKeyPressed ? '|' : '\\';
			break;
		case 0x2C:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, 'z', 'Z', 174, '<');
			break;
		case 0x2D:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, 'x', 'X', 175, '>');
			break;
		case 0x2E:
			localCharacter = isShiftKeyPressed ? 'C' : 'c';
			break;
		case 0x2F:
			localCharacter = isShiftKeyPressed ? 'V' : 'v';
			break;
		case 0x30:
			localCharacter = isShiftKeyPressed ? 'B' : 'b';
			break;
		case 0x31:
			localCharacter = isShiftKeyPressed ? 'N' : 'n';
			break;
		case 0x32:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, 'm', 'M', 230, -1);
			break;
		case 0x33:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, ',', '<', 196, -1);
			break;
		case 0x34:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, '.', '>', 249, -1);
			break;
		case 0x35:
			localCharacter = isShiftKeyPressed ? ':' : ';';
			break;
		case 0x73:
			localCharacter = getASCIICharacter(isShiftKeyPressed, altGraphKeyPressed, '/', '?', -1, 168);
			break;

		case 0x39:
			localCharacter = ' ';
			break;

		case 0x1C:
			localCharacter = '\r';
			break;

		case 0x47:
			localCharacter = '7';
			break;
		case 0x48:
			localCharacter = '8';
			break;
		case 0x49:
			localCharacter = '9';
			break;
		case 0x4B:
			localCharacter = '4';
			break;
		case 0x4C:
			localCharacter = '5';
			break;
		case 0x4D:
			localCharacter = '6';
			break;
		case 0x4F:
			localCharacter = '1';
			break;
		case 0x50:
			localCharacter = '2';
			break;
		case 0x51:
			localCharacter = '3';
			break;
		case 0x52:
			localCharacter = '0';
			break;

		case 0x37:
			localCharacter = '*';
			break;
		case 0x4A:
			localCharacter = '-';
			break;
		case 0x4E:
			localCharacter = '+';
			break;
		case 0x00:
			localCharacter = '.';
			break;
		case 0x53:
			localCharacter = ',';
			break;

		case 0xE035:
			localCharacter = '/';
			break;
		case 0xE01C:
			localCharacter = '\r';
			break;
	}

	if (localCharacter == -1) {
		return false;

	} else {
		*character = (char) localCharacter;
		return true;
	}
}
