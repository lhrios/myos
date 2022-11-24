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

#ifndef SYS_RESOURCE_H
	#define SYS_RESOURCE_H

	typedef unsigned int rlim_t;

	struct rlimit {
		rlim_t rlim_cur;
		rlim_t rlim_max;
	};

	#define RLIM_INFINITY 1

	#define RLIMIT_FSIZE 1
	#define RLIMIT_DATA 2
	#define RLIMIT_NOFILE 3

	int setrlimit(int, const struct rlimit*); // TODO: Implement me!
	int getrlimit(int, struct rlimit*); // TODO: Implement me!

#endif
