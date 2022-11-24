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
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#include "util/math_utils.h"

char* dirname(char* path) {
	size_t length = 0;
	int lastNameIndex = -1;
	int previousToLastSlashBlockIndex = -1;
	int lastSlashBlockIndex = -1;

	if (path != NULL) {
		while (path[length] != '\0') {
			if (path[length] == '/') {
				if (length == 0 || path[length - 1] != '/') {
					previousToLastSlashBlockIndex = lastSlashBlockIndex;
					lastSlashBlockIndex = length;
				}

			} else {
				if (length == 0 || path[length - 1] == '/') {
					lastNameIndex = length;
				}
			}
			length++;
		}
	}

	if (lastSlashBlockIndex != -1) {
		int end;
		if (lastNameIndex != -1) {
			if (lastSlashBlockIndex > lastNameIndex) {
				if (previousToLastSlashBlockIndex != -1) {
					end = previousToLastSlashBlockIndex;

				} else {
					end = lastNameIndex;
				}

			} else {
				end = lastSlashBlockIndex;
			}

		} else {
			end = lastSlashBlockIndex;
		}

		if (end == 0) {
			path[end + 1] = '\0';

		} else {
			path[end] = '\0';
		}

		return path;

	} else {
		return ".";
	}
}

char* basename(char* path) {
	size_t length = 0;
	int lastNameIndex = -1;
	int lastSlashBlockIndex = -1;

	if (path != NULL) {
		while (path[length] != '\0') {
			if (path[length] == '/') {
				if (length > 0 && path[length - 1] != '/') {
					lastSlashBlockIndex = length;
				}

			} else {
				if (length == 0 || path[length - 1] == '/') {
					lastNameIndex = length;
				}
			}
			length++;
		}
	}

	if (length == 0) {
		return ".";

	} else if (lastNameIndex == -1) {
		return "/";

	} else {
		if (lastSlashBlockIndex != -1 && lastSlashBlockIndex > lastNameIndex) {
			path[lastSlashBlockIndex] = '\0';
		}
		return &path[lastNameIndex];
	}
}
