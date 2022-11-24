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

#ifndef GRP_H
	#define GRP_H

	#include <sys/types.h>

	struct group {
		char* gr_name;
		gid_t gr_gid;
	   char** gr_mem;
	};

	struct group* getgrgid(gid_t gid);

	struct group* getgrnam(const char* groupName);

	struct group* getgrent(void);
   void setgrent(void);
   void endgrent(void);

#endif
