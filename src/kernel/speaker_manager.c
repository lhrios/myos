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

#include "kernel/busy_waiting_manager.h"
#include "kernel/keyboard.h"
#include "kernel/log.h"
#include "kernel/pit.h"
#include "kernel/speaker_manager.h"

/*
 * References:
 * - http://heim.ifi.uio.no/~inf3150/grupper/1/pcspeaker.html
 */
void speakerManagerPlaySound1(void) {
	int frequencies[] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494};
	#define FREQUENCIES_COUNT (sizeof(frequencies) / sizeof(int))

	keyboardSetSpeakerEnabled(true);

	for (int i = 0; i < FREQUENCIES_COUNT; i++) {
		pitConfigureSpeakerCounter(frequencies[i]);
		busyWaitingSleep(50);
	}
	keyboardSetSpeakerEnabled(false);
}

void speakerManagerBeep(uint32_t beepCount) {
	logDebug("Going to beep using the speaker");

	#define BEEP_FREQUENCY 494

	pitConfigureSpeakerCounter(BEEP_FREQUENCY);

	for (int i = 0; i < beepCount; i++) {
		bool last = i + 1 < beepCount;

		keyboardSetSpeakerEnabled(true);
		busyWaitingSleep(50);
		keyboardSetSpeakerEnabled(false);

		if (!last) {
			busyWaitingSleep(50);
		}
	}
}
