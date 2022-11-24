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

#ifndef PWD_H
	#define PWD_H

	#include <sys/types.h>

	struct passwd {
		char* pw_name; /* User's login name. */
		uid_t pw_uid; /* Numerical user ID. */
		gid_t pw_gid;  /*  Numerical group ID. */
		char* pw_dir;  /*  Initial working directory. */
		char* pw_shell; /* Program to use as shell. */
	};

	struct passwd* getpwuid(uid_t uid);

#endif
