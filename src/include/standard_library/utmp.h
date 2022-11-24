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

#ifndef UTMP_H
	#define UTMP_H

	#define UT_LINESIZE 32
	#define UT_NAMESIZE 32
	#define UT_HOSTSIZE 256

	// TODO: Implement me!

	struct exit_status {
		short e_termination;
		short e_exit;
	};

	struct utmp {
		short ut_type;
		pid_t ut_pid;
		char ut_line[UT_LINESIZE];
		char ut_id[4];

		char ut_user[UT_NAMESIZE];
		char ut_host[UT_HOSTSIZE];

		struct exit_status ut_exit;

		long ut_session;
		//struct timeval ut_tv;
		int32_t ut_addr_v6[4];
	};

#endif
