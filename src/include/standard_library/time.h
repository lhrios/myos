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

#ifndef TIME_H
	#define TIME_H

	#include <stdbool.h>

	#include <sys/times.h>
	#include <sys/types.h>

	struct tm {
		int tm_sec; /* Seconds [0,60]. */
		int tm_min; /* Minutes [0,59]. */
		int tm_hour; /* Hour [0,23].  */
		int tm_mday; /* Day of month [1,31]. */
		int tm_mon; /* Month of year [0,11].  */
		int tm_year; /* Years since 1900.  */
		int tm_wday; /* Day of week [0,6] (Sunday=0). */
		int tm_yday; /* Day of year [0,365].  */
		int tm_isdst; /* Daylight Savings flag. */
	};

	struct timespec {
		time_t tv_sec; /* Seconds. */
		long tv_nsec; /* Nanoseconds. */
	};

   extern char* tzname[2];

	time_t time(time_t* timeInstance);
	struct tm* gmtime(const time_t* timeInstance);
	struct tm* localtime(const time_t* timeInstance);
   void tzset(void);	// TODO: Implement me!
   size_t strftime(char* buffer, size_t bufferSize, const char* format, const struct tm* tmInstance);
   char* ctime(const time_t* timeInstance);
   char* asctime(const struct tm* tmInstance);

   clock_t clock(void);	// TODO: Implement me!
	#define CLOCKS_PER_SEC 1 // TODO: Implement me!

#endif
