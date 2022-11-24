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

#ifndef PATH_UTILS_H
	#define PATH_UTILS_H

	#include <stdbool.h>
	#include <stdint.h>
	#include <stdlib.h>

	#include "kernel/api_status_code.h"
	#include "kernel/memory_manager_constants.h"
	#include "kernel/limits.h"

	#define MAX_SEGMENTS_PER_PATH 446

	struct PathUtilsContext {
		char buffer[PATH_MAX_LENGTH];
		int segmentCount; /* It will not consider the last segment if the last segment has not been included. */
		const char* segments[MAX_SEGMENTS_PER_PATH];
		const char lastSegment[FILE_NAME_MAX_LENGTH]; /* Only changed if last path segment is not included on the final path. */
		bool hasLastSegment; /* Only changed if last path segment is not included on the final path. */
	};
	_Static_assert(sizeof(struct PathUtilsContext) <= PAGE_FRAME_SIZE, "The PathUtilsContext must fit inside a page frame.");

	bool pathUtilsIsAbsolute(const char* path);
	APIStatusCode pathUtilsCalculatePathSegments(struct PathUtilsContext* pathUtilsContext);
	APIStatusCode pathUtilsConcatenate(struct PathUtilsContext* pathUtilsContext, const char* path1, const char* path2, int path1Length, int path2Length);
	void pathUtilsNormalize(struct PathUtilsContext* pathUtilsContext);
	const char* pathUtilsFindNextPathSegment(const char* path, size_t* segmentLength);
	void pathUtilsCombinePathSegments(struct PathUtilsContext* pathUtilsContext, bool includeLastSegment);
	const char* pathUtilsFindLastPathSegment(const char* path, size_t pathLength, size_t* lastPathSegmentLength);
	APIStatusCode pathUtilsParsePath(struct PathUtilsContext* pathUtilsContext,
			const char* path, bool isPathNormalized, bool includeLastPathSegment,
			const char* currentWorkingDirectory, size_t currentWorkingDirectoryLength);

#endif
