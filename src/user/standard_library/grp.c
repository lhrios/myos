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

#include <grp.h>

static struct group groupInstance;
static char* members[] = {"root", NULL};
struct group* getgrgid(gid_t gid) {
	// TODO: Implement me!

	groupInstance.gr_name = "root";
	groupInstance.gr_gid = 0;
	groupInstance.gr_mem = members;

	return &groupInstance;
}

struct group* getgrnam(const char* groupName) {
	// TODO: Implement me!
	return getgrgid(0);
}

struct group* getgrent(void) {
	return NULL;
}

void setgrent(void) {
	// TODO: Implement me!
}

void endgrent(void) {
	// TODO: Implement me!
}
