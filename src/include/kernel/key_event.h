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

#ifndef KERNEL_KEY_EVENT_H
	#define KERNEL_KEY_EVENT_H

	#include <stdbool.h>
	#include <stdint.h>

	typedef uint32_t KeyEvent;

	#define ESCAPE_KEY_SCAN_CODE 0x01

	#define INSERT_KEY_SCAN_CODE 0xE052
	#define DELETE_KEY_SCAN_CODE 0xE053

	#define UP_ARROW_KEY_SCAN_CODE 0xE048
	#define DOWN_ARROW_KEY_SCAN_CODE 0xE050
	#define LEFT_ARROW_KEY_SCAN_CODE 0xE04B
	#define RIGHT_ARROW_KEY_SCAN_CODE 0xE04D

	#define PAGE_UP_KEY_SCAN_CODE 0xE049
	#define PAGE_DOWN_KEY_SCAN_CODE 0xE051

	#define HOME_KEY_SCAN_CODE 0xE047
	#define END_KEY_SCAN_CODE 0xE04F

	#define BACKSPACE_KEY_SCAN_CODE 0x0E

	#define G_KEY_SCAN_CODE 0x22

	#define F1_KEY_SCAN_CODE 0x3B
	#define F2_KEY_SCAN_CODE 0x3C
	#define F3_KEY_SCAN_CODE 0x3D
	#define F4_KEY_SCAN_CODE 0x3E

	#define F5_KEY_SCAN_CODE 0x3F
	#define F6_KEY_SCAN_CODE 0x40
	#define F7_KEY_SCAN_CODE 0x41
	#define F8_KEY_SCAN_CODE 0x42

	#define F9_KEY_SCAN_CODE 0x43
	#define F10_KEY_SCAN_CODE 0x44
	#define F11_KEY_SCAN_CODE 0x57
	#define F12_KEY_SCAN_CODE 0x58

	#define NUMBER_OF_F_KEYS 12
	extern uint32_t KEYBOARD_F_KEYS_SCAN_CODES[];

	#define KEY_EVENT_CONTROL_KEY_PRESSED_MASK (1 << 31)
	#define KEY_EVENT_ALT_KEY_PRESSED_MASK (1 << 30)
	#define KEY_EVENT_ALT_GRAPH_KEY_PRESSED_MASK (1 << 29)
	#define KEY_EVENT_SHIFT_KEY_PRESSED_MASK (1 << 28)
	#define KEY_EVENT_RELEASED_EVENT_MASK (1 << 27) /* Otherwise, the event was triggered in response to a key press. */
	#define KEY_EVENT_SCAN_CODE_MASK 0x07FFFFFF

	bool keyEventGetASCIICharacter(KeyEvent keyEvent, char* character);

#endif
