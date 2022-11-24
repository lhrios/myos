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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "kernel/busy_waiting_manager.h"
#include "kernel/interruption_manager.h"
#include "kernel/keyboard.h"
#include "kernel/log.h"
#include "kernel/pic.h"
#include "kernel/tty.h"
#include "kernel/x86.h"

/*
 * References:
 * - 8042 Keyboard Controller
 * - KBD42W11 Keyboard Controller
 * - "linux1.0/drivers/char/keyboard.c"
 * - http://www.osdever.net/papers/view/ibm-pc-keyboard-information-for-software-developers
 * - http://bochs.sourceforge.net/techspec/PORTS.LST
 */

#define DEFAULT_TIMEOUT 250 /* milliseconds */

#define KEYBOARD_CONTROLLER_DATA_PORT 0x60
#define KEYBOARD_CONTROLLER_PORT_B_PORT 0x61
#define KEYBOARD_CONTROLLER_COMMAND_PORT 0x64

struct KeyStroke {
	uint32_t keyId;
	uint32_t modifiersMask;
	bool onKeyRelease;
	bool (*callback)(void*);
	void* argument;
};

#define REGISTERED_KEY_STROKES_ARRAY_LENGTH 64
static struct KeyStroke registeredKeyStrokes[REGISTERED_KEY_STROKES_ARRAY_LENGTH];
static uint32_t registeredKeyStrokesCount = 0;

static uint8_t keyboardInterruptionVector;

static bool hasLastScanCode = false;
static uint8_t lastScanCode = 0;

static bool isShiftKeyPressed = false;
static bool isControlKeyPressed = false;
static bool isAltGraphKeyPressed = false;
static bool isAltKeyPressed = false;

static bool isNumericLockOn = false;
static bool isCapitalLettersLockOn = false;
static bool isScrollLockOn = false;

static bool isIgnoringInput = true;

static void (*keyEventSink)(KeyEvent) = NULL;

static bool isOutputBufferFull(void) {
	return x86InputByteFromPort(KEYBOARD_CONTROLLER_COMMAND_PORT) & 0x1;
}

static bool isKeyRelease(int byteCount, uint8_t firstByte, uint8_t secondByte) {
	if (byteCount == 1) {
		return (firstByte & 0x80) != 0 ? true : false;

	} else {
		assert(byteCount == 2);
		assert(firstByte == 0xE0);

		return (secondByte & 0x80) != 0 ? true : false;
	}
}

static bool waitWhileInputBufferIsNotEmpty(uint32_t milliseconds) {
	bool hasTimeLeft = true;
	uint64_t before = 0;
	if (milliseconds > 0) {
		before = x86Rdtsc();
	}
	bool isEmpty;
	do {
		isEmpty = (x86InputByteFromPort(KEYBOARD_CONTROLLER_COMMAND_PORT) & 0x2) == 0;
	} while (!isEmpty && (milliseconds == 0 || (hasTimeLeft = busyWaitingHasTimeLeft(before, milliseconds))));

	return !hasTimeLeft;
}

static bool waitWhileOutputBufferIsFull(uint32_t milliseconds) {
	bool hasTimeLeft = true;
	uint64_t before = 0;
	if (milliseconds > 0) {
		before = x86Rdtsc();
	}
	bool isFull;
	do {
		isFull = isOutputBufferFull();
	} while (isFull && (milliseconds == 0 || (hasTimeLeft = busyWaitingHasTimeLeft(before, milliseconds))));

	return !hasTimeLeft;
}

static bool handleModificationKeys(int byteCount, uint8_t firstByte, uint8_t secondByte) {
	if (byteCount == 1) {
		switch (0x7F & firstByte) {
			case 0x2A:
			case 0x36:
				isShiftKeyPressed = !isKeyRelease(byteCount, firstByte, secondByte);
				return true;
			case 0x1D:
				isControlKeyPressed = !isKeyRelease(byteCount, firstByte, secondByte);
				return true;
			case 0x38:
				isAltKeyPressed = !isKeyRelease(byteCount, firstByte, secondByte);
				return true;
			case 0x45:
				if (!isKeyRelease(byteCount, firstByte, secondByte)) {
					isNumericLockOn = !isNumericLockOn;
					keyboardSetModeIndicators(isNumericLockOn, isCapitalLettersLockOn, isScrollLockOn);
				}
				return true;
			case 0x3A:
				if (!isKeyRelease(byteCount, firstByte, secondByte)) {
					isCapitalLettersLockOn = !isCapitalLettersLockOn;
					keyboardSetModeIndicators(isNumericLockOn, isCapitalLettersLockOn, isScrollLockOn);
				}
				return true;
			case 0x46:
				if (!isKeyRelease(byteCount, firstByte, secondByte)) {
					isScrollLockOn = !isScrollLockOn;
					keyboardSetModeIndicators(isNumericLockOn, isCapitalLettersLockOn, isScrollLockOn);
				}
				return true;
		}

	} else {
		assert(byteCount == 2);
		assert(firstByte == 0xE0);

		switch (0x7F & secondByte) {
			case 0x1D:
				isControlKeyPressed = !isKeyRelease(byteCount, firstByte, secondByte);
				return true;
			case 0x38:
				isAltGraphKeyPressed = !isKeyRelease(byteCount, firstByte, secondByte);
				return true;
		}
	}

	return false;
}

static bool areModifiersBeingFulfilled(uint32_t modifiersMask) {
	if ((modifiersMask & KEY_EVENT_CONTROL_KEY_PRESSED_MASK) != 0 && !isControlKeyPressed) {
		return false;
	}
	if ((modifiersMask & KEY_EVENT_ALT_KEY_PRESSED_MASK) != 0 && !isAltKeyPressed) {
		return false;
	}
	if ((modifiersMask & KEY_EVENT_ALT_GRAPH_KEY_PRESSED_MASK) != 0 && !isAltGraphKeyPressed) {
		return false;
	}
	if ((modifiersMask & KEY_EVENT_SHIFT_KEY_PRESSED_MASK) != 0 && !isShiftKeyPressed) {
		return false;
	}

	return true;
}

static bool triggerKeyStrokesCallbacks(uint32_t keyId, bool keyReleased) {
	uint32_t triggerCount = 0;
	for (uint32_t i = 0; i < registeredKeyStrokesCount; i++) {
		struct KeyStroke *keyStroke = &registeredKeyStrokes[i];

		if (keyStroke->keyId == keyId
				&& areModifiersBeingFulfilled(keyStroke->modifiersMask)
				&& keyStroke->onKeyRelease == keyReleased) {
			if (keyStroke->callback(keyStroke->argument)) {
				triggerCount++;
			}
		}
	}

	return triggerCount != 0;
}

static void issueEndOfKeyboardIRQ(void) {
	picIssueEndOfInterrupt(IRQ1, false);
}

static KeyEvent createKeyEvent(bool keyReleased, uint8_t firstScanCode, uint8_t secondScanCode) {
	KeyEvent keyEvent = 0;
	if (isShiftKeyPressed) {
		keyEvent |= KEY_EVENT_SHIFT_KEY_PRESSED_MASK;
	}
	if (isAltGraphKeyPressed) {
		keyEvent |= KEY_EVENT_ALT_GRAPH_KEY_PRESSED_MASK;
	}
	if (isAltKeyPressed) {
		keyEvent |= KEY_EVENT_ALT_KEY_PRESSED_MASK;
	}
	if (isControlKeyPressed) {
		keyEvent |= KEY_EVENT_CONTROL_KEY_PRESSED_MASK;
	}
	if (keyReleased) {
		keyEvent |= KEY_EVENT_RELEASED_EVENT_MASK;
	}
	if (firstScanCode == 0xE0) {
		keyEvent |= (0xE0 << 8) | secondScanCode;
	} else {
		keyEvent |= firstScanCode;
	}
	return keyEvent;
}

static void afterHandlingKeyboardIRQ(bool isHardwareEnabled) {
	if (!isHardwareEnabled) {
		waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT);
		x86OutputByteToPort(KEYBOARD_CONTROLLER_COMMAND_PORT, 0xAE); /* Enable keyboard feature. */
	}
	issueEndOfKeyboardIRQ();
}

static void handleKeyboardIRQ(uint32_t errorCode, struct ProcessExecutionState1* processExecutionState1, struct ProcessExecutionState2* processExecutionState2) {
	assert(!picIsSpuriousIRQ(IRQ1));
	assert(processExecutionState2->interruptionVector == keyboardInterruptionVector);

	bool isHardwareEnabled = true;
	if (!waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT)) {
		x86OutputByteToPort(KEYBOARD_CONTROLLER_COMMAND_PORT, 0xAD); /* Disable keyboard feature. */
		waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT);
		isHardwareEnabled = false;
	}
	interruptionManagerRegisterCommandToRunAfterInterruptionHandler(PRIORITY_LOWEST, (void(*)(void*)) &afterHandlingKeyboardIRQ, (void*) (int) isHardwareEnabled);

	/* Is output buffer full? */
	if (isOutputBufferFull()) {
		uint8_t scanCode = x86InputByteFromPort(KEYBOARD_CONTROLLER_DATA_PORT);

		/* Used to debug easily. */
		if (false) {
			logDebug("Keyboard scan code: %x", scanCode);

		} else {
			if (!hasLastScanCode) {
				if (scanCode != 0xE0) {
					if (!isIgnoringInput && scanCode != 0xFA) { /* Ignore the acknowledge. */
						if (!handleModificationKeys(1, scanCode, 0)) {
							bool keyReleased = isKeyRelease(1, scanCode, 0);
							scanCode = scanCode & 0x7F;
							if (!triggerKeyStrokesCallbacks(scanCode, keyReleased)) {
								if (keyEventSink != NULL) {
									keyEventSink(createKeyEvent(keyReleased, scanCode, 0));
								}
							}
						}
					}

				} else {
					hasLastScanCode = true;
					lastScanCode = scanCode;
				}

			} else {
				if (lastScanCode == 0xE0) {
					if (!isIgnoringInput) {
						if (!handleModificationKeys(2, lastScanCode, scanCode)) {
							bool keyReleased = isKeyRelease(2, lastScanCode, scanCode);
							scanCode = scanCode & 0x7F;
							uint32_t completeScanCode = (lastScanCode << 8) | scanCode;
							if (!triggerKeyStrokesCallbacks(completeScanCode, keyReleased)) {
								if (keyEventSink != NULL) {
									keyEventSink(createKeyEvent(keyReleased, lastScanCode, scanCode));
								}
							}
						}
					}
				}
				hasLastScanCode = false;
			}
		}
	}
}

void keyboardInitialize(uint8_t newKeyboardInterruptionVector) {
	keyboardInterruptionVector = newKeyboardInterruptionVector;
	interruptionManagerRegisterInterruptionHandler(keyboardInterruptionVector, &handleKeyboardIRQ);
}

void keyboardSetModeIndicators(bool numericLock, bool capitalLettersLock, bool scrollLock) {
	waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT);
	x86OutputByteToPort(KEYBOARD_CONTROLLER_DATA_PORT, 0xED);
	waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT);
	x86OutputByteToPort(KEYBOARD_CONTROLLER_DATA_PORT, ((capitalLettersLock ? 1 : 0) << 2) | ((numericLock ? 1 : 0) << 1) | (scrollLock ? 1 : 0));
	waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT);
}

void keyboardSetKeyEventSink(void (*newKeyEventSink)(KeyEvent)) {
	keyEventSink = newKeyEventSink;
}

void keyboardInitializeHardware(void) {
	while (isOutputBufferFull()) {
		x86InputByteFromPort(KEYBOARD_CONTROLLER_DATA_PORT);
	}

	waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT);
	x86OutputByteToPort(KEYBOARD_CONTROLLER_COMMAND_PORT, 0x20);
	waitWhileOutputBufferIsFull(DEFAULT_TIMEOUT);
	uint8_t commandByte = x86InputByteFromPort(KEYBOARD_CONTROLLER_DATA_PORT);

	waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT);
	x86OutputByteToPort(KEYBOARD_CONTROLLER_COMMAND_PORT, 0x60);
	x86OutputByteToPort(KEYBOARD_CONTROLLER_DATA_PORT, (commandByte & 0xE7) | 0x08);

	waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT);
	x86OutputByteToPort(KEYBOARD_CONTROLLER_COMMAND_PORT, 0xAE); /* Enable keyboard feature. */

	waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT);
	x86OutputByteToPort(KEYBOARD_CONTROLLER_DATA_PORT, 0xF3);
	waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT);
	x86OutputByteToPort(KEYBOARD_CONTROLLER_DATA_PORT, (0x1 << 5) | 0x0); /* Set Typematic Rate/Delay. */

	isNumericLockOn = true;
	isCapitalLettersLockOn = false;
	isScrollLockOn = false;
	keyboardSetModeIndicators(isNumericLockOn, isCapitalLettersLockOn , isScrollLockOn);
}

bool keyboardRegisterKeyStroke(uint32_t keyId, uint32_t modifiersMask,
		bool onKeyRelease, bool (*callback)(void*), void* argument) {
	if (registeredKeyStrokesCount < REGISTERED_KEY_STROKES_ARRAY_LENGTH) {
		struct KeyStroke *keyStroke = &registeredKeyStrokes[registeredKeyStrokesCount];
		registeredKeyStrokesCount++;

		keyStroke->keyId = keyId;
		keyStroke->modifiersMask = modifiersMask;
		keyStroke->onKeyRelease = onKeyRelease;
		keyStroke->callback = callback;
		keyStroke->argument = argument;

		return true;

	} else {
		return false;
	}
}

void keyboardReboot(void) {
	uint64_t before = 0;
	before = x86Rdtsc();
	bool isFull;
	do {
		isFull = isOutputBufferFull();
		if (isFull) {
			x86InputByteFromPort(KEYBOARD_CONTROLLER_DATA_PORT);
		}
	} while (isFull && busyWaitingHasTimeLeft(before, DEFAULT_TIMEOUT));

	waitWhileInputBufferIsNotEmpty(DEFAULT_TIMEOUT);
	x86OutputByteToPort(KEYBOARD_CONTROLLER_COMMAND_PORT, 0xFE);

	/* If something goes wrong. */
	x86Cli();
	x86Ring0Stop();
}

void keyboardSetSpeakerEnabled(bool enable) {
	uint8_t value = x86InputByteFromPort(KEYBOARD_CONTROLLER_PORT_B_PORT);
	if (enable) {
		value |= 0x03; /* It also connects PIT counter 2 with the speaker. */
	} else {
		value &= 0xFC; /* It also connects PIT counter 2 with the speaker. */
	}
	x86OutputByteToPort(KEYBOARD_CONTROLLER_PORT_B_PORT, value);
}

void keyboardSetIsIgnoringInput(bool newIsIgnoringInput) {
	isIgnoringInput = newIsIgnoringInput;
}
