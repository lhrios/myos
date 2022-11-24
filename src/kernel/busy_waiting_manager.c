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

#include <stdbool.h>
#include <stdint.h>

#include "kernel/busy_waiting_manager.h"
#include "kernel/log.h"
#include "kernel/pic.h"
#include "kernel/pit.h"
#include "kernel/x86.h"

#include "util/math_utils.h"

static uint32_t cyclesPerMillisecond;

#define ITERATION_COUNT_BEFORE_STOP 5

static bool initialized = false;
static uint32_t iterationCount;
static volatile bool stop;
static void chronometerCallback(void) {
	iterationCount++;
	if (iterationCount == ITERATION_COUNT_BEFORE_STOP) {
		stop = true;
	} else {
		pitStartChronometer(0, &chronometerCallback);
	}
}

bool busyWaitingHasTimeLeft(uint64_t before, uint32_t milliseconds) {
	uint64_t cycles = (uint64_t) milliseconds * (uint64_t) cyclesPerMillisecond;
	return x86Rdtsc() - before < cycles;
}

uint32_t busyWaitingCalculateTime(uint64_t before, uint64_t after) {
	return ((uint32_t) (after - before)) / cyclesPerMillisecond;
}

void busyWaitingSleep(uint32_t milliseconds) {
	if (initialized) {
		uint64_t cycles = (uint64_t) milliseconds * (uint64_t) cyclesPerMillisecond;
		uint64_t cyclesElapsed;
		uint64_t before = x86Rdtsc();
		do {
			cyclesElapsed =  x86Rdtsc() - before;
		} while (cyclesElapsed < cycles);

	} else {
		/* It has not been initialized yet. It will do its best. */
		for (int i = 0; i < milliseconds; i++) {
			for (int j = 0; j < 0xFFFFF; j++) {
				__asm__ __volatile__(
					"nop"
					:
					:
					: "memory");
			}
		}
	}
}

void busyWaitingManagerInitialize(void) {
	stop = false;
	iterationCount = 0;

	uint64_t before = x86Rdtsc();
	pitStartChronometer(0, &chronometerCallback);
	while (!stop);
	uint64_t after = x86Rdtsc();

	cyclesPerMillisecond = ((uint32_t)(after - before)) /
			(55 /* time (in milliseconds) to count up to 65536 at a frequency of 1193182 Hz */ * ITERATION_COUNT_BEFORE_STOP);
	cyclesPerMillisecond = mathUtilsMax(1, cyclesPerMillisecond);

	initialized = true;

	logDebug("%d cyclesPerMillisecond", cyclesPerMillisecond);
}
