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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "kernel/cmos.h"
#include "kernel/log.h"
#include "kernel/x86.h"
#include "util/date_time_utils.h"

/*
 * References:
 * - http://www.bioscentral.com/misc/cmosmap.htm
 * - https://wiki.osdev.org/CMOS
 */

#define REGISTER_SELECTION_PORT 0x70
#define INPUT_OUTPUT_PORT 0x71

#define STATUS_REGISTER_A_ID 0x0A
#define STATUS_REGISTER_B_ID 0x0B

#define STATUS_SECONDS_ID 0x00
#define STATUS_MINUTES_ID 0x02
#define STATUS_HOURS_ID 0x04
#define STATUS_DAY_ID 0x07
#define STATUS_MONTH_ID 0x08
#define STATUS_YEAR_ID 0x09

static time_t initializationTime;

static uint8_t convertFromBCDToBinary(uint8_t bcdValue) {
	return ((bcdValue / 16) * 10) + (bcdValue & 0xf);
}

static uint8_t readRegister(uint8_t registerId) {
	/* As we do not want to disable the non-maskable interrupt (NMI). */
	x86OutputByteToPort(REGISTER_SELECTION_PORT, registerId & ~0x80);
	return x86InputByteFromPort(INPUT_OUTPUT_PORT);
}

static bool isEncodedUsingBCD(void) {
	return (readRegister(STATUS_REGISTER_B_ID) & 0x4) == 0;
}

static bool dateTimeCanBeRead(void) {
	x86OutputByteToPort(REGISTER_SELECTION_PORT, STATUS_REGISTER_A_ID);
	return (x86InputByteFromPort(INPUT_OUTPUT_PORT) & 0x80) == 0;
}

static void readDateTime(struct tm* tmInstance) {
	tmInstance->tm_sec = readRegister(STATUS_SECONDS_ID);
	tmInstance->tm_min = readRegister(STATUS_MINUTES_ID);
	tmInstance->tm_hour = readRegister(STATUS_HOURS_ID);

	tmInstance->tm_mday = readRegister(STATUS_DAY_ID);
	tmInstance->tm_mon = readRegister(STATUS_MONTH_ID);
	tmInstance->tm_year = readRegister(STATUS_YEAR_ID);
}

time_t cmosGetUnixTime(void) {
	struct tm tmInstance1;
	struct tm tmInstance2;

	memset(&tmInstance1, 0, sizeof(struct tm));
	memset(&tmInstance2, 0, sizeof(struct tm));

	do {
		while (!dateTimeCanBeRead());
		readDateTime(&tmInstance1);
		while (!dateTimeCanBeRead());
		readDateTime(&tmInstance2);
	} while (!dateTimeUtilsAreTmInstancesEqual(&tmInstance1, &tmInstance2));

	if (isEncodedUsingBCD()) {
		tmInstance1.tm_sec = convertFromBCDToBinary(tmInstance1.tm_sec);
		tmInstance1.tm_min = convertFromBCDToBinary(tmInstance1.tm_min);
		tmInstance1.tm_hour = convertFromBCDToBinary(tmInstance1.tm_hour);

		tmInstance1.tm_mday = convertFromBCDToBinary(tmInstance1.tm_mday);
		tmInstance1.tm_mon = convertFromBCDToBinary(tmInstance1.tm_mon);
		tmInstance1.tm_year = convertFromBCDToBinary(tmInstance1.tm_year);
	}
	tmInstance1.tm_mon--;
	tmInstance1.tm_year += 2000 - 1900;
	assert(tmInstance1.tm_mon >= 0);

	return dateTimeUtilsTmInstanceToUnixTime(&tmInstance1);
}

time_t cmosGetInitializationTime(void) {
	return initializationTime;
}

void cmosInitialize(void) {
	initializationTime = cmosGetUnixTime();

	struct tm tmInstance;
	dateTimeUtilsUnixTimeToTmInstance(initializationTime, &tmInstance);

	const int bufferSize = 128;
	char buffer[bufferSize];
	strftime(buffer, bufferSize, "%F %T %z", &tmInstance);
	logDebug("Initialization time (%d) is %s", initializationTime, buffer);
}
