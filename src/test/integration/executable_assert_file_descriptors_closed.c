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
#include <errno.h>
#include <unistd.h>

#include <sys/stat.h>

int main(int argc, char** argv) {
	assert(argc > 0);
	for (int i = 0; i < argc; i++) {
		int fileDescriptorIndex = atoi(argv[i]);
		struct stat statInstance;
		int result = fstat(fileDescriptorIndex, &statInstance);
		assert(result == -1);
		assert(errno == EBADF);
	}
	return EXIT_SUCCESS;
}
