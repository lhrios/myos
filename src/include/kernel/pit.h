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

#ifndef KERNEL_PIT_H
	#define KERNEL_PIT_H

	#include <stdbool.h>
	#include <stdint.h>

	void pitInitialize(uint8_t newPITInterruptionVector);
	void pitStartChronometer(uint32_t count, void (*newChronometerCallback)(void));
	void pitStartTickGenerator(uint32_t frequency);
	void pitStopTickGenerator();
	void pitConfigureSpeakerCounter(uint32_t frequency);
	bool pitRegisterCommandToRunOnTick(void (*command)(uint64_t, uint64_t));
	uint64_t pitGetUpTimeInMilliseconds(void);

#endif
