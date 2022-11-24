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
#include <stddef.h>
#include <stdint.h>

#include "kernel/command_scheduler.h"
#include "kernel/interruption_manager.h"
#include "kernel/log.h"
#include "kernel/pit.h"
#include "kernel/pic.h"
#include "kernel/x86.h"

/*
 * References:
 * - 82C54 CHMOS Programable Internal Timer
 * - https://wiki.osdev.org/Programmable_Interval_Timer - Programmable Interval Timer
 */

#define PIT_DATA_BUS 0x40
#define PIT_DATA_BUS_A0_0_A1_0 (PIT_DATA_BUS + 0)
#define PIT_DATA_BUS_A0_0_A1_1 (PIT_DATA_BUS + 1)
#define PIT_DATA_BUS_A0_1_A1_0 (PIT_DATA_BUS + 2)
#define PIT_DATA_BUS_A0_1_A1_1 (PIT_DATA_BUS + 3)

#define PIT_MODE_0_TERMINAL_COUNT 0x0
#define PIT_MODE_2_RATE_GENERATOR 0x4
#define PIT_MODE_3_SQUARE_WAVE_MODE 0x6

/*
 * Begin of data related with counter 0.
 */
static uint64_t tickCount;
static uint32_t millisecondsBetweenTicks;
static uint8_t pitInterruptionVector;

static bool counter0IsEnabled;
struct CommandToRunOnTick {
	void (*command)(uint64_t, uint64_t);
};
void (*chronometerCallback)();

#define REGISTERED_COMMANDS_TO_RUN_ON_TICK_ARRAY_LENGTH 64
static struct CommandToRunOnTick registeredCommandsToRunOnTick[REGISTERED_COMMANDS_TO_RUN_ON_TICK_ARRAY_LENGTH];
static uint32_t registeredCommandsToRunOnTickCount = 0;
/*
 * End of data related with counter.
 */

static void issueEndOfPITIRQ(void) {
	picIssueEndOfInterrupt(IRQ0, false);
}

static int indexOfNextTickCommandToRun = 0;
static void runTickCommandAndScheduleNext(int* index) {
	if (*index < registeredCommandsToRunOnTickCount) {
		struct CommandToRunOnTick* commandsToRunOnTick = &registeredCommandsToRunOnTick[*index];

		/* As the command may not return now (because a different process may be scheduled to run), we schedule the action before running the command. */
		(*index)++;
		interruptionManagerRegisterCommandToRunAfterInterruptionHandler(PRIORITY_HIGH, (void(*)(void*)) &runTickCommandAndScheduleNext, &indexOfNextTickCommandToRun);

		commandsToRunOnTick->command(tickCount, pitGetUpTimeInMilliseconds());
	}
}

static void handlePITIRQ(uint32_t errorCode, struct ProcessExecutionState1* processExecutionState1, struct ProcessExecutionState2* processExecutionState2) {
	assert(!picIsSpuriousIRQ(IRQ0));
	assert(processExecutionState2->interruptionVector == pitInterruptionVector);

	if (counter0IsEnabled) {
		if (chronometerCallback == NULL) {
			tickCount++;

			interruptionManagerRegisterCommandToRunAfterInterruptionHandler(PRIORITY_LOWEST, (void(*)(void*)) &issueEndOfPITIRQ, NULL);
			indexOfNextTickCommandToRun = 0;
			interruptionManagerRegisterCommandToRunAfterInterruptionHandler(PRIORITY_HIGH, (void(*)(void*)) &runTickCommandAndScheduleNext, &indexOfNextTickCommandToRun);

		} else {
			void (*oldChronometerCallback)() = chronometerCallback;

			counter0IsEnabled = false;
			chronometerCallback = NULL;

			oldChronometerCallback();

			issueEndOfPITIRQ();
		}
	} else {
		logWarn("Unexpected IRQ for counter 0");
	}
}

static void pitConfigureCounter(uint8_t counter, uint32_t frequencyOrCount, uint8_t mode) {
	assert(counter <= 2);
	assert(mode == PIT_MODE_0_TERMINAL_COUNT || mode == PIT_MODE_2_RATE_GENERATOR || mode == PIT_MODE_3_SQUARE_WAVE_MODE);

	uint32_t value;
	if (mode == PIT_MODE_0_TERMINAL_COUNT) {
		value = frequencyOrCount;
	} else {
		value = 1193182 / frequencyOrCount;
		assert(frequencyOrCount <= 1000);
		millisecondsBetweenTicks = 1000 / frequencyOrCount;
	}
	assert((value & 0xFFFF0000) >> 16 == 0);

	if (counter == 0) {
		counter0IsEnabled = true;
	} else {
		assert(counter == 2);
	}

	x86OutputByteToPort(PIT_DATA_BUS_A0_1_A1_1, 0x30 | mode | (counter << 6)); /* read/write least significant byte first, then most significant byte | binary counter 16-bits. */
	uint16_t port = 0;
	switch(counter) {
		case 0:
				port = PIT_DATA_BUS_A0_0_A1_0;
			break;
		case 1:
				port = PIT_DATA_BUS_A0_0_A1_1;
			break;
		case 2:
				port = PIT_DATA_BUS_A0_1_A1_0;
			break;
	}

	x86OutputByteToPort(port, value & 0xFF); /* Write least significant byte. */
	x86OutputByteToPort(port, (value >> 8) & 0xFF); /* Write most significant byte. */
}

void pitInitialize(uint8_t newPITInterruptionVector) {
	pitInterruptionVector = newPITInterruptionVector;
	interruptionManagerRegisterInterruptionHandler(pitInterruptionVector, &handlePITIRQ);

	/* It disables the counter 0. It is done by doing a very fast count. */
	uint32_t count = 1;
	x86OutputByteToPort(PIT_DATA_BUS_A0_1_A1_1, 0x30 | PIT_MODE_0_TERMINAL_COUNT | (0 << 6)); /* read/write least significant byte first, then most significant byte | binary counter 16-bits. */
	x86OutputByteToPort(PIT_DATA_BUS_A0_0_A1_0, count & 0xFF); /* Write least significant byte. */
	x86OutputByteToPort(PIT_DATA_BUS_A0_0_A1_0, (count >> 8) & 0xFF); /* Write most significant byte. */
}

/**
 * A very simple chronometer that can be used only while the kernel is initializing itself.
 */
void pitStartChronometer(uint32_t count, void (*newChronometerCallback)()) {
	assert(count <= 0xFFFF);
	assert(newChronometerCallback != NULL);
	assert(!counter0IsEnabled);

	chronometerCallback = newChronometerCallback;
	pitConfigureCounter(0, count, PIT_MODE_0_TERMINAL_COUNT);
}

void pitStartTickGenerator(uint32_t frequency) {
	pitConfigureCounter(0, frequency, PIT_MODE_2_RATE_GENERATOR);
}

void pitStopTickGenerator() {
	pitConfigureCounter(0, 1, PIT_MODE_0_TERMINAL_COUNT);
	counter0IsEnabled = false;
}

void pitConfigureSpeakerCounter(uint32_t frequency) {
	pitConfigureCounter(2, frequency, PIT_MODE_3_SQUARE_WAVE_MODE);
}

uint64_t pitGetUpTimeInMilliseconds(void) {
	return tickCount * millisecondsBetweenTicks;
}

bool pitRegisterCommandToRunOnTick(void (*command)(uint64_t, uint64_t)) {
	if (registeredCommandsToRunOnTickCount + 1 < REGISTERED_COMMANDS_TO_RUN_ON_TICK_ARRAY_LENGTH) {
		struct CommandToRunOnTick* commandsToRunOnTick = &registeredCommandsToRunOnTick[registeredCommandsToRunOnTickCount++];
		commandsToRunOnTick->command = command;
		return true;

	} else {
		return false;
	}
}
