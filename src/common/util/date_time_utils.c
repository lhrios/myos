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
#include <limits.h>

#include "util/date_time_utils.h"
#include "util/math_utils.h"

#define MINUTE_DURATION_IN_SECONDS 60

#define HOUR_DURATION_IN_MINUTES 60
#define HOUR_DURATION_IN_SECONDS (HOUR_DURATION_IN_MINUTES * MINUTE_DURATION_IN_SECONDS)

#define DAY_DURATION_IN_HOURS 24
#define DAY_DURATION_IN_SECONDS (DAY_DURATION_IN_HOURS * HOUR_DURATION_IN_SECONDS)

#define YEAR_DURATION_IN_MONTHS 12

#define REGULAR_YEAR_DURATION_IN_DAYS 365
#define LEAP_YEAR_DURATION_IN_DAYS 366

static int MONTH_DURATION_IN_DAYS_LEAP_YEAR[] = {
		31, /* January */
		29, /* February */
		31, /* March */
		30, /* April */
		31, /* May */
		30, /* June */
		31, /* July */
		31, /* August */
		30, /* September */
		31, /* October */
		30, /* November */
		31 /* December */
};

static int MONTH_DURATION_IN_DAYS_REGULAR_YEAR[] = {
		31, /* January */
		28, /* February */
		31, /* March */
		30, /* April */
		31, /* May */
		30, /* June */
		31, /* July */
		31, /* August */
		30, /* September */
		31, /* October */
		30, /* November */
		31 /* December */
};

static bool isLeapYear(int year) {
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static int* getMonthDurationInDays(int year) {
	if (isLeapYear(year)) {
		return MONTH_DURATION_IN_DAYS_LEAP_YEAR;
	} else {
		return MONTH_DURATION_IN_DAYS_REGULAR_YEAR;
	}
}

bool dateTimeUtilsAreTmInstancesEqual(struct tm* tmInstance1, struct tm* tmInstance2) {
	return tmInstance1->tm_sec == tmInstance2->tm_sec && tmInstance1->tm_min == tmInstance2->tm_min && tmInstance1->tm_hour == tmInstance2->tm_hour
		&& tmInstance1->tm_mday == tmInstance2->tm_mday && tmInstance1->tm_mon == tmInstance2->tm_mon && tmInstance1->tm_year == tmInstance2->tm_year
		&& tmInstance1->tm_wday == tmInstance2->tm_wday && tmInstance1->tm_yday == tmInstance2->tm_yday && tmInstance1->tm_isdst == tmInstance2->tm_isdst;
}

time_t dateTimeUtilsTmInstanceToUnixTime(struct tm* tmInstance) {
	int year = tmInstance->tm_year + 1900;
	time_t result = year / 4 - year / 100 + year / 400 + (year - 1) * REGULAR_YEAR_DURATION_IN_DAYS - 719162
		- (isLeapYear(year) ? 1 : 0);
	result *= DAY_DURATION_IN_SECONDS;

	int* monthDurationInDays = getMonthDurationInDays(tmInstance->tm_year);
	for (int i = 0; i < tmInstance->tm_mon; i++) {
		result += monthDurationInDays[i] * DAY_DURATION_IN_SECONDS;
	}
	result += (tmInstance->tm_mday - 1) * DAY_DURATION_IN_SECONDS;

	result += tmInstance->tm_hour * HOUR_DURATION_IN_SECONDS + tmInstance->tm_min * MINUTE_DURATION_IN_SECONDS + tmInstance->tm_sec;

	return result;
}

static void calculateHourMinuteSecond(int seconds, struct tm* tmInstance) {
	tmInstance->tm_sec = seconds % MINUTE_DURATION_IN_SECONDS;
	tmInstance->tm_min = (seconds % HOUR_DURATION_IN_SECONDS) / MINUTE_DURATION_IN_SECONDS;
	tmInstance->tm_hour = (seconds % DAY_DURATION_IN_SECONDS) / HOUR_DURATION_IN_SECONDS;
}

void dateTimeUtilsUnixTimeToTmInstance(time_t unixTime, struct tm* tmInstance) {
	tmInstance->tm_isdst = 0;

	int dayOfYear;
	if (unixTime < 0) {
		int dayRemainder = (unixTime % DAY_DURATION_IN_SECONDS) + DAY_DURATION_IN_SECONDS;
		calculateHourMinuteSecond(dayRemainder, tmInstance);

		int days = mathUtilsCeilOfUint32Division(-unixTime, DAY_DURATION_IN_SECONDS);
		tmInstance->tm_wday = (7 - (days % 7) + 4) % 7;

		int year = 1970 - mathUtilsCeilOfUint32Division(days * 100, REGULAR_YEAR_DURATION_IN_DAYS * 100 + 25);
		tmInstance->tm_year = year - 1900;
		dayOfYear = 719162 - (year / 4 - year / 100 + year / 400 + (year - 1) * REGULAR_YEAR_DURATION_IN_DAYS)
			- days + (isLeapYear(year) ? 1 : 0);

	} else {
		calculateHourMinuteSecond(unixTime, tmInstance);

		int days = unixTime / DAY_DURATION_IN_SECONDS;
		tmInstance->tm_wday = (4 + (days % 7)) % 7;

		int year = (days * 100) / (REGULAR_YEAR_DURATION_IN_DAYS * 100 + 25) + 1970;
		tmInstance->tm_year = year - 1900;
		dayOfYear = days - (year / 4 - year / 100 + year / 400 + (year - 1) * REGULAR_YEAR_DURATION_IN_DAYS - 719162)
			+ (isLeapYear(year) ? 1 : 0);
	}
	tmInstance->tm_yday = dayOfYear;

	int* monthDurationInDays = getMonthDurationInDays(tmInstance->tm_year);
	for (tmInstance->tm_mon = 0; true; tmInstance->tm_mon++) {
		assert(tmInstance->tm_mon < YEAR_DURATION_IN_MONTHS);
		if (dayOfYear > monthDurationInDays[tmInstance->tm_mon]) {
			dayOfYear -= monthDurationInDays[tmInstance->tm_mon];
		} else {
			break;
		}
	}
	assert(dayOfYear >= 0);
	tmInstance->tm_mday = dayOfYear + 1;
}
