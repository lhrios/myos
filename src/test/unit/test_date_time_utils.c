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
#include <limits.h>
#include <time.h>

#include "util/date_time_utils.h"

#include <stdio.h>

static void testDateTimeUtilsTmInstanceToUnixTime(void) {
	/* GMT: Saturday, 3 January 1976 02:43:57 */
	struct tm tmInstance;
	tmInstance.tm_year = 1976 - 1900;
	tmInstance.tm_mon = 0;
	tmInstance.tm_mday = 3;
	tmInstance.tm_hour = 2;
	tmInstance.tm_min = 43;
	tmInstance.tm_sec = 57;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == 189485037);

	/* GMT: Monday, 21 May 1934 22:43:57 */
	tmInstance.tm_year = 1934 - 1900;
	tmInstance.tm_mon = 4;
	tmInstance.tm_mday = 21;
	tmInstance.tm_hour = 22;
	tmInstance.tm_min = 43;
	tmInstance.tm_sec = 57;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == -1123895763);

	/* GMT: Thursday, 1 January 1970 00:00:00 */
	tmInstance.tm_year = 1970 - 1900;
	tmInstance.tm_mon = 0;
	tmInstance.tm_mday = 1;
	tmInstance.tm_hour = 0;
	tmInstance.tm_min = 0;
	tmInstance.tm_sec = 0;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == 0);

	/* GMT: Sunday, 1 January 1950 00:00:00 */
	tmInstance.tm_year = 1950 - 1900;
	tmInstance.tm_mon = 0;
	tmInstance.tm_mday = 1;
	tmInstance.tm_hour = 0;
	tmInstance.tm_min = 0;
	tmInstance.tm_sec = 0;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == -631152000);

	/* GMT: Tuesday, 5 February 2019 23:05:59 */
	tmInstance.tm_year = 2019 - 1900;
	tmInstance.tm_mon = 1;
	tmInstance.tm_mday = 5;
	tmInstance.tm_hour = 23;
	tmInstance.tm_min = 5;
	tmInstance.tm_sec = 59;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == 1549407959);

	/* GMT: Thursday, 24 October 1929 08:25:38 */
	tmInstance.tm_year = 1929 - 1900;
	tmInstance.tm_mon = 9;
	tmInstance.tm_mday = 24;
	tmInstance.tm_hour = 8;
	tmInstance.tm_min = 25;
	tmInstance.tm_sec = 38;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == -1268235262);

	/* GMT: Friday, 31 December 1976 23:59:59 */
	tmInstance.tm_year = 1976 - 1900;
	tmInstance.tm_mon = 11;
	tmInstance.tm_mday = 31;
	tmInstance.tm_hour = 23;
	tmInstance.tm_min = 59;
	tmInstance.tm_sec = 59;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == 220924799);

	/* GMT: Tuesday, 29 February 2000 11:22:33 */
	tmInstance.tm_year = 2000 - 1900;
	tmInstance.tm_mon = 1;
	tmInstance.tm_mday = 29;
	tmInstance.tm_hour = 11;
	tmInstance.tm_min = 22;
	tmInstance.tm_sec = 33;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == 951823353);

	/* GMT: Saturday, 21 March 1903 19:52:32 */
	tmInstance.tm_year = 1903 - 1900;
	tmInstance.tm_mon = 2;
	tmInstance.tm_mday = 21;
	tmInstance.tm_hour = 19;
	tmInstance.tm_min = 52;
	tmInstance.tm_sec = 32;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == -2107483648);

	/* GMT: Friday, 13 December 1901 20:45:52 */
	tmInstance.tm_year = 1901 - 1900;
	tmInstance.tm_mon = 11;
	tmInstance.tm_mday = 13;
	tmInstance.tm_hour = 20;
	tmInstance.tm_min = 45;
	tmInstance.tm_sec = 52;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == -2147483648);

	/* GMT: Tuesday, 19 January 2038 03:14:07 */
	tmInstance.tm_year = 2038 - 1900;
	tmInstance.tm_mon = 0;
	tmInstance.tm_mday = 19;
	tmInstance.tm_hour = 3;
	tmInstance.tm_min = 14;
	tmInstance.tm_sec = 7;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == INT_MAX);

	/* GMT: Saturday, 29 February 2020 11:59:55 */
	tmInstance.tm_year = 2020 - 1900;
	tmInstance.tm_mon = 1;
	tmInstance.tm_mday = 29;
	tmInstance.tm_hour = 11;
	tmInstance.tm_min = 59;
	tmInstance.tm_sec = 55;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == 1582977595);

	/* GMT: Thursday, 24 October 1929 08:25:38 */
	tmInstance.tm_year = 1929 - 1900;
	tmInstance.tm_mon = 9;
	tmInstance.tm_mday = 24;
	tmInstance.tm_hour = 8;
	tmInstance.tm_min = 25;
	tmInstance.tm_sec = 38;
	assert(dateTimeUtilsTmInstanceToUnixTime(&tmInstance) == -1268235262);
}

static void testDateTimeUtilsUnixTimeToTmInstance(void) {
	struct tm tmInstance;
	time_t unixTime;

	{
		unixTime = 1660802943; /* GMT: Thursday, 18 August 2022 06:09:03 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 3);
		assert(tmInstance.tm_min == 9);
		assert(tmInstance.tm_hour == 6);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 4);
		assert(tmInstance.tm_yday == 229);

		assert(tmInstance.tm_mday == 18);
		assert(tmInstance.tm_mon == 7);
		assert(tmInstance.tm_year + 1900 == 2022);
	}

	{
		unixTime = 1590883195; /* GMT: Saturday, 30 May 2020 23:59:55 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 55);
		assert(tmInstance.tm_min == 59);
		assert(tmInstance.tm_hour == 23);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 6);
		assert(tmInstance.tm_yday == 150);

		assert(tmInstance.tm_mday == 30);
		assert(tmInstance.tm_mon == 4);
		assert(tmInstance.tm_year + 1900 == 2020);
	}

	{
		unixTime = 1582977595; /* GMT: Saturday, 29 February 2020 11:59:55 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 55);
		assert(tmInstance.tm_min == 59);
		assert(tmInstance.tm_hour == 11);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 6);
		assert(tmInstance.tm_yday == 59);

		assert(tmInstance.tm_mday == 29);
		assert(tmInstance.tm_mon == 1);
		assert(tmInstance.tm_year + 1900 == 2020);
	}

	{
		unixTime = INT_MAX; /* GMT: Tuesday, 19 January 2038 03:14:07 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 7);
		assert(tmInstance.tm_min == 14);
		assert(tmInstance.tm_hour == 3);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 2);
		assert(tmInstance.tm_yday == 18);

		assert(tmInstance.tm_mday == 19);
		assert(tmInstance.tm_mon == 0);
		assert(tmInstance.tm_year + 1900 == 2038);
	}

	{
		unixTime = 220924799; /* GMT: Friday, 31 December 1976 23:59:59 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 59);
		assert(tmInstance.tm_min == 59);
		assert(tmInstance.tm_hour == 23);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 5);
		assert(tmInstance.tm_yday == 365);

		assert(tmInstance.tm_mday == 31);
		assert(tmInstance.tm_mon == 11);
		assert(tmInstance.tm_year + 1900 == 1976);
	}

	{
		unixTime = 0; /* GMT: Thursday, 1 January 1970 00:00:00 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 0);
		assert(tmInstance.tm_min == 0);
		assert(tmInstance.tm_hour == 0);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 4);
		assert(tmInstance.tm_yday == 0);

		assert(tmInstance.tm_mday == 1);
		assert(tmInstance.tm_mon == 0);
		assert(tmInstance.tm_year + 1900 == 1970);
	}

	{
		unixTime = -211375504; /* GMT: Sunday, 21 April 1963 12:34:56 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 56);
		assert(tmInstance.tm_min == 34);
		assert(tmInstance.tm_hour == 12);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 0);
		assert(tmInstance.tm_yday == 110);

		assert(tmInstance.tm_mday == 21);
		assert(tmInstance.tm_mon == 3);
		assert(tmInstance.tm_year + 1900 == 1963);
	}

	{
		unixTime = -2147483648; /* GMT: Friday, 13 December 1901 20:45:52 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 52);
		assert(tmInstance.tm_min == 45);
		assert(tmInstance.tm_hour == 20);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 5);
		assert(tmInstance.tm_yday == 346);

		assert(tmInstance.tm_mday == 13);
		assert(tmInstance.tm_mon == 11);
		assert(tmInstance.tm_year + 1900 == 1901);
	}

	{
		unixTime = -2107483648; /* GMT: Saturday, 21 March 1903 19:52:32 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 32);
		assert(tmInstance.tm_min == 52);
		assert(tmInstance.tm_hour == 19);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 6);
		assert(tmInstance.tm_yday == 79);

		assert(tmInstance.tm_mday == 21);
		assert(tmInstance.tm_mon == 2);
		assert(tmInstance.tm_year + 1900 == 1903);
	}

	{
		unixTime = -1; /* GMT: Wednesday, 31 December 1969 23:59:59 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 59);
		assert(tmInstance.tm_min == 59);
		assert(tmInstance.tm_hour == 23);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 3);
		assert(tmInstance.tm_yday == 364);

		assert(tmInstance.tm_mday == 31);
		assert(tmInstance.tm_mon == 11);
		assert(tmInstance.tm_year + 1900 == 1969);
	}

	{
		unixTime = -799623070; /* GMT: Wednesday, 30 August 1944 02:28:50 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 50);
		assert(tmInstance.tm_min == 28);
		assert(tmInstance.tm_hour == 2);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 3);
		assert(tmInstance.tm_yday == 242);

		assert(tmInstance.tm_mday == 30);
		assert(tmInstance.tm_mon == 7);
		assert(tmInstance.tm_year + 1900 == 1944);
	}

	{
		unixTime = -978316791; /* GMT: Saturday, 31 December 1938 21:20:09 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 9);
		assert(tmInstance.tm_min == 20);
		assert(tmInstance.tm_hour == 21);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 6);
		assert(tmInstance.tm_yday == 364);

		assert(tmInstance.tm_mday == 31);
		assert(tmInstance.tm_mon == 11);
		assert(tmInstance.tm_year + 1900 == 1938);
	}

	{
		unixTime = -1262304000; /* GMT: Wednesday, 1 January 1930 00:00:00 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 0);
		assert(tmInstance.tm_min == 0);
		assert(tmInstance.tm_hour == 0);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 3);
		assert(tmInstance.tm_yday == 0);

		assert(tmInstance.tm_mday == 1);
		assert(tmInstance.tm_mon == 0);
		assert(tmInstance.tm_year + 1900 == 1930);
	}

	{
		unixTime = -31536000; /* GMT: Wednesday, 1 January 1969 00:00:00 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 0);
		assert(tmInstance.tm_min == 0);
		assert(tmInstance.tm_hour == 0);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 3);
		assert(tmInstance.tm_yday == 0);

		assert(tmInstance.tm_mday == 1);
		assert(tmInstance.tm_mon == 0);
		assert(tmInstance.tm_year + 1900 == 1969);
	}

	{
		unixTime = -1268235262; /* GMT: Thursday, 24 October 1929 08:25:38 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 38);
		assert(tmInstance.tm_min == 25);
		assert(tmInstance.tm_hour == 8);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 4);
		assert(tmInstance.tm_yday == 296);

		assert(tmInstance.tm_mday == 24);
		assert(tmInstance.tm_mon == 9);
		assert(tmInstance.tm_year + 1900 == 1929);
	}

	{
		unixTime = 951823353; /* GMT: Tuesday, 29 February 2000 11:22:33 */
		dateTimeUtilsUnixTimeToTmInstance(unixTime, &tmInstance);

		assert(tmInstance.tm_sec == 33);
		assert(tmInstance.tm_min == 22);
		assert(tmInstance.tm_hour == 11);

		assert(tmInstance.tm_isdst == 0);
		assert(tmInstance.tm_wday == 2);
		assert(tmInstance.tm_yday == 59);

		assert(tmInstance.tm_mday == 29);
		assert(tmInstance.tm_mon == 1);
		assert(tmInstance.tm_year + 1900 == 2000);
	}
}

int main(int argc, char** argv) {
	testDateTimeUtilsTmInstanceToUnixTime();
	testDateTimeUtilsUnixTimeToTmInstance();
	return 0;
}
