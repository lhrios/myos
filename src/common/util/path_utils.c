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
#include <string.h>

#include "kernel/limits.h"

#include "util/path_utils.h"

static const char* dotDot = "..";
static const char* dot = ".";

bool pathUtilsIsAbsolute(const char* path) {
	return path[0] == '/';
}

APIStatusCode pathUtilsConcatenate(struct PathUtilsContext* pathUtilsContext, const char* path1, const char* path2, int path1Length, int path2Length) {
	if (path1Length < 0) {
		path1Length = strlen(path1);
	}
	if (path2Length < 0) {
		path2Length = strlen(path2);
	}

	if (path1Length >= 1 && path1[path1Length - 1] == '/') {
		path1Length--;
	}

	if (path2Length >= 1 && path2[0] == '/') {
		path2++;
		path2Length--;
	}

	if (path1Length + 1 + path2Length + 1 <= PATH_MAX_LENGTH) {
		strncpy(pathUtilsContext->buffer, path1, path1Length);
		pathUtilsContext->buffer[path1Length] = '/';
		strncpy(pathUtilsContext->buffer + path1Length + 1, path2, path2Length);
		pathUtilsContext->buffer[path1Length + 1 + path2Length] = '\0';
		return SUCCESS;
	} else {
		return ENAMETOOLONG;
	}
}

APIStatusCode pathUtilsCalculatePathSegments(struct PathUtilsContext* pathUtilsContext) {
	int segmentCount = 0;
	int dotCount = 0;
	int segmentLength = 0;
	bool onlyDots = true;
	const char* segmentBegin = pathUtilsContext->buffer;
	char lastCharacter;
	char* nextCharacter = pathUtilsContext->buffer;
	do {
		switch (lastCharacter = *nextCharacter) {
			case '.':
				dotCount++;
				segmentLength++;
				break;

			case '\0':
			case '/':
				if (segmentLength > 0) {
					if (segmentCount >= MAX_SEGMENTS_PER_PATH) {
						return ENOMEM;
					}

					if (onlyDots && dotCount <= 2) {
						if (dotCount == 1) {
							pathUtilsContext->segments[segmentCount++] =  dot;
						} else {
							pathUtilsContext->segments[segmentCount++] =  dotDot;
						}
					} else if (nextCharacter - segmentBegin <= FILE_NAME_MAX_LENGTH - 1) {
						pathUtilsContext->segments[segmentCount++] =  segmentBegin;
						if (*nextCharacter) {
							*nextCharacter = '\0';
						}
					} else {
						return ENAMETOOLONG;
					}
				}

				dotCount = 0;
				segmentLength = 0;
				onlyDots = true;
				segmentBegin = nextCharacter + 1;
				break;

			default:
				onlyDots = false;
				segmentLength++;
				break;
		}

		nextCharacter++;
	} while (lastCharacter != '\0');

	pathUtilsContext->segmentCount = segmentCount;
	return SUCCESS;
}

void pathUtilsNormalize(struct PathUtilsContext* pathUtilsContext) {
	int stackTopIndex = 0;
	int nextSegmentIndex = 0;

	while (nextSegmentIndex < pathUtilsContext->segmentCount) {
		if (strcmp(pathUtilsContext->segments[nextSegmentIndex], dotDot) == 0) {
			if (stackTopIndex - 1 >= 0) {
				stackTopIndex--;
			}

		} else if (strcmp(pathUtilsContext->segments[nextSegmentIndex], dot) == 0) {
			/* Nothing to do */

		} else {
			pathUtilsContext->segments[stackTopIndex++] = pathUtilsContext->segments[nextSegmentIndex];
		}

		nextSegmentIndex++;
	}

	pathUtilsContext->segmentCount = stackTopIndex;
}

void pathUtilsCombinePathSegments(struct PathUtilsContext* pathUtilsContext, bool includeLastSegment) {
	char* normalizedPath = pathUtilsContext->buffer;
	if (pathUtilsContext->segmentCount == 0) {
		*normalizedPath++ = '/';
		*normalizedPath = '\0';
		pathUtilsContext->segmentCount = 1;
		if (!includeLastSegment) {
			pathUtilsContext->hasLastSegment = false;
		}

	} else {
		int segmentCount;
		if (includeLastSegment) {
			segmentCount = pathUtilsContext->segmentCount;
			pathUtilsContext->hasLastSegment = false;

		} else {
			segmentCount = pathUtilsContext->segmentCount - 1;
			pathUtilsContext->hasLastSegment = true;
			strcpy((char*) pathUtilsContext->lastSegment, pathUtilsContext->segments[segmentCount]);
		}

		if (segmentCount > 0) {
			for (int i = 0; i < segmentCount; i++) {
				*normalizedPath++ = '/';
				for (int j = 0; pathUtilsContext->segments[i][j] != '\0'; j++) {
					*normalizedPath++ = pathUtilsContext->segments[i][j];
				}
			}
			pathUtilsContext->segmentCount = segmentCount;

		} else {
			*normalizedPath++ = '/';
			pathUtilsContext->segmentCount = 1;
		}
		*normalizedPath++ = '\0';
	}
}

const char* pathUtilsFindNextPathSegment(const char* path, size_t* segmentLength) {
	uint32_t segmentStart = 0;
	uint32_t segmentEnd = 0;

	char c;
	for (int i = 0; (c = path[i]) != '\0'; i++) {
		if (c == '/') {
			if (segmentEnd == 0) {
				segmentStart++;
				segmentEnd++;
			} else {
				break;
			}
		} else {
			segmentEnd++;
		}
	}

	*segmentLength = segmentEnd - segmentStart;
	return path + segmentStart;
}

const char* pathUtilsFindLastPathSegment(const char* path, size_t pathLength, size_t* lastPathSegmentLength) {
	int i;
	for (i = pathLength - 1; i >= 0 && path[i] != '/'; i--);
	if (lastPathSegmentLength != NULL) {
		*lastPathSegmentLength = pathLength - i - 1;
	}
	return path + i + 1;
}

APIStatusCode pathUtilsParsePath(struct PathUtilsContext* pathUtilsContext,
		const char* path, bool isPathNormalized, bool includeLastPathSegment,
		const char* currentWorkingDirectory, size_t currentWorkingDirectoryLength) {
	APIStatusCode result = SUCCESS;

	size_t pathLength = strlen(path);
	if (pathLength + 1 > PATH_MAX_LENGTH) {
		result = ENAMETOOLONG;

	} else {
		if (isPathNormalized) {
			assert(pathUtilsIsAbsolute(path));
			strcpy(pathUtilsContext->buffer, path);

			pathUtilsContext->hasLastSegment = false;
			if (!includeLastPathSegment) {
				size_t lastPathSegmentLength;
				const char* lastPathSegment = pathUtilsFindLastPathSegment(pathUtilsContext->buffer, pathLength, &lastPathSegmentLength);
				if (lastPathSegmentLength != 0) {
					pathUtilsContext->hasLastSegment = true;
					strcpy((char*) pathUtilsContext->lastSegment, lastPathSegment);
					pathUtilsContext->buffer[pathLength - lastPathSegmentLength - 1] = '\0';
				}
			}

		} else {
			if (pathUtilsIsAbsolute(path)) {
				strcpy(pathUtilsContext->buffer, path);
			} else {
				result = pathUtilsConcatenate(pathUtilsContext, currentWorkingDirectory, path, currentWorkingDirectoryLength, pathLength);
			}

			if (result == SUCCESS) {
				result = pathUtilsCalculatePathSegments(pathUtilsContext);
			}

			if (result == SUCCESS) {
				pathUtilsNormalize(pathUtilsContext);
				pathUtilsCombinePathSegments(pathUtilsContext, includeLastPathSegment);
			}
		}
	}

	return result;
}
