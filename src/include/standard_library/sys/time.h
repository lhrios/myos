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

#ifndef SYS_TIME_H
	#define SYS_TIME_H

	#include <sys/types.h>

	// TODO: Implement me!

	struct timeval {
		time_t tv_sec; /* Seconds. */
		suseconds_t tv_usec; /* Microseconds. */
	};

	struct timezone {
		int tz_minuteswest;
		int tz_dsttime;
	};

	int gettimeofday(struct timeval* tv, struct timezone* tz);

#endif
