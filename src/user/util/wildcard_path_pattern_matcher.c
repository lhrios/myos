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
#include <dirent.h>
#include <errno.h>
#include <glob.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "user/util/dynamic_array_utils.h"
#include "user/util/wildcard_pattern_matcher.h"
#include "user/util/wildcard_path_pattern_matcher.h"

#include "util/string_stream_writer.h"

//#define DEBUG_WILDCARD_PATH_PATTERN_MATCHER

/*
 * Define a buffer length large enough to handle the case in which the path is too long (exceeds PATH_MAX).
 * The length is able to store the first occurrence (during a breadth-first search) of a path considered too
 * long according to the criterion described above.
 */
#define STRING_STREAM_WRITER_BUFFER_LENGTH (PATH_MAX /* (path + '/') */ \
	+ FILE_NAME_MAX_LENGTH /* (file name + '\0') */ \
)

struct PathSegment {
	bool isWildcardPattern;

	const char* prefixBegin;
	int prefixLength;

	const char* payloadBegin;
	int payloadLength;
};

struct WildcardPathPatternMatcherContext {
	int flags;
	int (*errorCallBack)(const char*, int);

	bool isAbsolute;
	const char* pattern;
	size_t patternLength;

	struct DynamicArray pathSegments;

	char* stringStreamWriterBuffer;
	struct StringStreamWriter stringStreamWriter;
};

static struct PathSegment* pushPathSegment(struct WildcardPathPatternMatcherContext* context) {
	struct PathSegment pathSegment;
	memset(&pathSegment, 0, sizeof(struct PathSegment));

	if (dynamicArrayInsertAfterLast(&context->pathSegments, &pathSegment)) {
		int pathSegmentCount = dynamicArraySize(&context->pathSegments);
		struct PathSegment* pathSegments = dynamicArrayGetArray(&context->pathSegments);
		return &pathSegments[pathSegmentCount - 1];

	} else {
		return NULL;
	}
}

static struct PathSegment* peekPathSegment(struct WildcardPathPatternMatcherContext* context) {
	int pathSegmentCount = dynamicArraySize(&context->pathSegments);
	struct PathSegment* pathSegments = dynamicArrayGetArray(&context->pathSegments);

	if (pathSegmentCount > 0) {
		return &pathSegments[pathSegmentCount - 1];
	} else {
		return NULL;
	}
}

static int matchSegmentPayloadCharacter(struct WildcardPathPatternMatcherContext* context, const char* pattern, int patternLength) {
	struct PathSegment* currentPathSegment = peekPathSegment(context);
	if (currentPathSegment == NULL) {
		currentPathSegment = pushPathSegment(context);
		if (currentPathSegment == NULL) {
			return GLOB_NOSPACE;
		}
	}
	if (currentPathSegment->prefixBegin == NULL) {
		currentPathSegment->prefixBegin = &pattern[patternLength];
	}
	if (currentPathSegment->payloadBegin == NULL) {
		currentPathSegment->payloadBegin = &pattern[patternLength];
	}
	currentPathSegment->payloadLength++;

	return 0;
}

static int handleNonFatalErrors(struct WildcardPathPatternMatcherContext* context, const char* path) {
	int result = 0;

	if (context->errorCallBack != NULL) {
		result = context->errorCallBack(path, errno);
	}

	if ((context->flags & GLOB_ERR) != 0) {
		result = GLOB_ABORTED;
	}

	return result;
}

static int expandDirectory(struct WildcardPathPatternMatcherContext* context, const char* path, int pathSegmentIndex, bool prependWithPath,
		struct DynamicArray* output) {
	#ifdef DEBUG_WILDCARD_PATH_PATTERN_MATCHER
		fprintf(stderr, "expandDirectory(%s, %d, %d, %p)\n", path, pathSegmentIndex, prependWithPath, output);
	#endif

	int pathSegmentCount = dynamicArraySize(&context->pathSegments);
	struct PathSegment* pathSegments = dynamicArrayGetArray(&context->pathSegments);

	int error = 0;
	/* Is it the last? */
	bool last = pathSegmentIndex + 1 == pathSegmentCount;
	size_t pathLength = strlen(path);
	struct PathSegment* pathSegment = &pathSegments[pathSegmentIndex];

	char* rawPattern = strndup(pathSegment->payloadBegin, pathSegment->payloadLength);
	if (rawPattern != NULL) {
		#ifdef DEBUG_WILDCARD_PATH_PATTERN_MATCHER
			fprintf(stderr, "\tpattern=%s\n", rawPattern);
		#endif
	} else {
		error = GLOB_NOSPACE;
	}

	struct WildcardPattern pattern;
	if (error == 0) {
		int flags = FNM_PERIOD;
		if (context->flags & GLOB_NOESCAPE) {
			flags |= FNM_NOESCAPE;
		}
		error = wildcardPatternMatcherParsePattern(rawPattern, &pattern, flags) ? 0 : GLOB_NOSPACE;
		free(rawPattern);
	}

	if (error == 0) {
		DIR* directory = opendir(path);
		if (directory != NULL) {
			while (error == 0) {
				errno = 0;
				struct dirent* entry = readdir(directory);
				if (entry == NULL) {
					if (errno != 0) {
						error = handleNonFatalErrors(context, path);
					}
					break;

				} else {
					#ifdef DEBUG_WILDCARD_PATH_PATTERN_MATCHER
						fprintf(stderr, "\tevaluating=%s\n", entry->d_name);
					#endif

					size_t entryNameLength = strlen(entry->d_name);

					struct StringStreamWriter* stringStreamWriter = &context->stringStreamWriter;
					struct StreamWriter* streamWriter = &context->stringStreamWriter.streamWriter;

					assert(pathLength + 1 + entryNameLength + 1 <= STRING_STREAM_WRITER_BUFFER_LENGTH);
					streamWriterWrite(streamWriter, path, pathLength);
					streamWriterWriteCharacter(streamWriter, '/');
					streamWriterWrite(streamWriter, entry->d_name, entryNameLength);
					streamWriterWriteCharacter(streamWriter, '\0');

					struct stat statInstance;
					if (stat(stringStreamWriterGetBuffer(stringStreamWriter), &statInstance) == 0) {
						if ((last || S_ISDIR(statInstance.st_mode) || S_ISLNK(statInstance.st_mode))
								&& wildcardPatternMatcherMatch(entry->d_name, &pattern)) {
							bool appendSlash = last && S_ISDIR(statInstance.st_mode) && (context->flags & GLOB_MARK) != 0;
							size_t nameLength = strlen(entry->d_name);
							size_t childLength = 0;
							{
								childLength += prependWithPath ? pathLength : 0;
								childLength += pathSegment->prefixLength;
								childLength += nameLength;
								childLength += appendSlash ? 1 : 0;
								childLength += 1;
							}
							char* childPath = malloc(sizeof(char) * childLength);
							if (childPath != NULL) {
								size_t offset = 0;
								if (prependWithPath) {
									strncpy(childPath, path, pathLength);
									offset += pathLength;
								}
								strncpy(childPath + offset, pathSegment->prefixBegin, pathSegment->prefixLength);
								offset += pathSegment->prefixLength;
								strncpy(childPath + offset, entry->d_name, nameLength);
								offset += nameLength;
								if (appendSlash) {
									strcpy(childPath + offset, "/");
								} else {
									childPath[offset] = '\0';
								}

								#ifdef DEBUG_WILDCARD_PATH_PATTERN_MATCHER
									fprintf(stderr, "\tadding '%s'\n", childPath);
								#endif
								error = dynamicArrayInsertAfterLast(output, &childPath) ? 0 : GLOB_NOSPACE;

							} else {
								error = GLOB_NOSPACE;
							}
						}

					} else {
						error = handleNonFatalErrors(context, stringStreamWriterGetBuffer(stringStreamWriter));
					}
				}
			}
			closedir(directory);

		} else {
			if (errno == ENOMEM) {
				error = GLOB_NOSPACE;
			} else if (errno != ENOTDIR) {
				error = handleNonFatalErrors(context, path);
			}
		}

		wildcardPatternMatcherReleasePattern(&pattern);
	}

	return error;
}

static struct DynamicArray* getExpandDirectoryOutput(struct WildcardPathPatternMatcherContext* context, int pathSegmentIndex,
		struct DynamicArray* local, struct DynamicArray* result) {
	if (pathSegmentIndex + 1 >= dynamicArraySize(&context->pathSegments)) {
		return result;
	} else {
		return local;
	}
}

static int breadthFirstSearch(struct WildcardPathPatternMatcherContext* context, struct DynamicArray* result) {
	int error = 0;

	struct DynamicArray currentFrontierInstance;
	dynamicArrayInitialize(&currentFrontierInstance, sizeof(char*), NULL,
		(void (*)(void*, void*)) &dynamicArrayUtilsMemoryAllocatorRelease,
		(void* (*)(void*, void*, size_t)) &dynamicArrayUtilsMemoryAllocatorResize);

	struct DynamicArray nextFrontierInstance;
	dynamicArrayInitialize(&nextFrontierInstance, sizeof(char*), NULL,
		(void (*)(void*, void*)) &dynamicArrayUtilsMemoryAllocatorRelease,
		(void* (*)(void*, void*, size_t)) &dynamicArrayUtilsMemoryAllocatorResize);

	struct DynamicArray* currentFrontier = &currentFrontierInstance;
	struct DynamicArray* nextFrontier = &nextFrontierInstance;

	#ifdef DEBUG_WILDCARD_PATH_PATTERN_MATCHER
		fprintf(stderr, "%p %p %p\n", currentFrontier, nextFrontier, result);
	#endif

	int initialPathSegmentIndex;
	if (error == 0) {
		/* Initialize: */
		int pathSegmentCount = dynamicArraySize(&context->pathSegments);
		struct PathSegment* pathSegments = dynamicArrayGetArray(&context->pathSegments);

		for (initialPathSegmentIndex = 0; initialPathSegmentIndex < pathSegmentCount; initialPathSegmentIndex++) {
			struct PathSegment* currentPathSegment = &pathSegments[initialPathSegmentIndex];
			if (currentPathSegment->isWildcardPattern) {
				break;
			}
		}
		struct DynamicArray* output = getExpandDirectoryOutput(context, initialPathSegmentIndex, currentFrontier, result);

		if (initialPathSegmentIndex == 0) {
			if (context->isAbsolute) {
				error = expandDirectory(context, "/", initialPathSegmentIndex, false, output);
			} else {
				error = expandDirectory(context, ".", initialPathSegmentIndex, false, output);
			}

		} else if (initialPathSegmentIndex < pathSegmentCount) {
			char* path = strndup(pathSegments[0].prefixBegin,
				(int) (pathSegments[initialPathSegmentIndex].prefixBegin - pathSegments[0].prefixBegin));
			if (path != NULL) {
				error = expandDirectory(context, path, initialPathSegmentIndex, true, output);
				free(path);
			} else {
				error = GLOB_NOSPACE;
			}

		} else {
			struct stat statInstance;
			int statResult = stat(context->pattern, &statInstance);
			if (statResult == -1) {
				error = handleNonFatalErrors(context, context->pattern);

			} else {
				char* path = malloc(context->patternLength + 1 + 1);
				if (path != NULL) {
					strcpy(path, context->pattern);
					if (S_ISDIR(statInstance.st_mode) && (context->flags & GLOB_MARK) != 0) {
						strcpy(path + context->patternLength, "/");
					}
					error = dynamicArrayInsertAfterLast(output, &path) ? 0 : GLOB_NOSPACE;

				} else {
					error = GLOB_NOSPACE;
				}
			}
		}

		/* Expand frontier: */
		for (int pathSegmentIndex = initialPathSegmentIndex + 1; pathSegmentIndex < pathSegmentCount && error == 0;
				pathSegmentIndex++) {
			struct DynamicArray* output = getExpandDirectoryOutput(context, pathSegmentIndex, nextFrontier, result);
			if (dynamicArraySize(currentFrontier) > 0) {
				char** array = dynamicArrayGetArray(currentFrontier);
				for (int i = 0; i < dynamicArraySize(currentFrontier); i++) {
					char* path = array[i];
					if (error == 0) {
						error = expandDirectory(context, path, pathSegmentIndex, true, output);
					}
					free(array[i]);
				}
			}
			dynamicArrayClear(currentFrontier, false);

			struct DynamicArray* aux;
			aux = currentFrontier;
			currentFrontier = nextFrontier;
			nextFrontier = aux;
		}
	}

	dynamicArrayClear(currentFrontier, true);
	dynamicArrayClear(nextFrontier, true);

	return error;
}

static int stringComparator(const char** item1, const char** item2) {
	return strcmp(*item1, *item2);
}

static int parsePathSegments(struct WildcardPathPatternMatcherContext* context) {
	int error = 0;

	const char* pattern = context->pattern;

	char c;
	int patternLength = 0;
	while ((c = pattern[patternLength]) != '\0' && error == 0) {
		switch (c) {
			case '/': {
				if (patternLength == 0) {
					context->isAbsolute = true;
				}
				struct PathSegment* currentPathSegment = peekPathSegment(context);
				if (currentPathSegment == NULL) {
					currentPathSegment = pushPathSegment(context);
					if (currentPathSegment != NULL) {
						currentPathSegment->prefixBegin = &pattern[patternLength];
						currentPathSegment->prefixLength++;
					} else {
						error = GLOB_NOSPACE;
					}

				} else {
					if (currentPathSegment->payloadLength > 0) {
						currentPathSegment = pushPathSegment(context);
						if (currentPathSegment != NULL) {
							currentPathSegment->prefixBegin = &pattern[patternLength];
							currentPathSegment->prefixLength++;
						} else {
							error = GLOB_NOSPACE;
						}

					} else {
						currentPathSegment->prefixLength++;
					}
				}
			} break;

			default:
				matchSegmentPayloadCharacter(context, pattern, patternLength);
				if ((c == '*' || c == '[' || c == '?' || c == '\\')) {
					struct PathSegment* currentPathSegment = peekPathSegment(context);
					assert(currentPathSegment != NULL);
					currentPathSegment->isWildcardPattern = true;
				}
				break;
		}

		patternLength++;
	}

	if (error == 0) {
		/* Remove the trailing '/'. */
		if (dynamicArraySize(&context->pathSegments) > 1) {
			struct PathSegment* pathSegment = peekPathSegment(context);
			if (pathSegment->payloadLength == 0) {
				dynamicArrayRemoveLast(&context->pathSegments);
			}
		}

		context->patternLength = patternLength;
	}

	return error;
}

int wildcardPathPatternMatcherMatch(const char* pattern, int flags,
		int (*errorCallBack)(const char*, int), glob_t* globInstance) {
	int resultSummary = 0;

	struct DynamicArray* result = &globInstance->result;
	if ((flags & GLOB_APPEND) != 0) {
		if (dynamicArraySize(result) > 0) {
			dynamicArrayRemoveLast(result);
		}
	} else {
		dynamicArrayInitialize(result, sizeof(char*), NULL,
			(void (*)(void*, void*)) &dynamicArrayUtilsMemoryAllocatorRelease,
			(void* (*)(void*, void*, size_t)) &dynamicArrayUtilsMemoryAllocatorResize);
		globInstance->slotsReservedAtTheBeginning = 0;
	}

	struct WildcardPathPatternMatcherContext context;
	memset(&context, 0, sizeof(struct WildcardPathPatternMatcherContext));
	context.flags = flags;
	context.pattern = pattern;
	context.errorCallBack = errorCallBack;
	dynamicArrayInitialize(&context.pathSegments, sizeof(struct PathSegment), NULL,
		(void (*)(void*, void*)) &dynamicArrayUtilsMemoryAllocatorRelease,
		(void* (*)(void*, void*, size_t)) &dynamicArrayUtilsMemoryAllocatorResize);
	context.stringStreamWriterBuffer = malloc(sizeof(char) * STRING_STREAM_WRITER_BUFFER_LENGTH);
	if (context.stringStreamWriterBuffer == NULL) {
		resultSummary = GLOB_NOSPACE;
	} else {
		stringStreamWriterInitialize(&context.stringStreamWriter, context.stringStreamWriterBuffer, STRING_STREAM_WRITER_BUFFER_LENGTH);
	}

	if (resultSummary == 0) {
		resultSummary = parsePathSegments(&context);
	}

	if (resultSummary == 0) {
		#ifdef DEBUG_WILDCARD_PATH_PATTERN_MATCHER
			{
				int pathSegmentCount = dynamicArraySize(&context.pathSegments);
				struct PathSegment* pathSegments = dynamicArrayGetArray(&context.pathSegments);

				for (int i = 0; i < pathSegmentCount; i++) {
					struct PathSegment* currentPathSegment = &pathSegments[i];
					fprintf(stderr, "[%d]:\n", i);
					fprintf(stderr, "\tisWildcardPattern=%d\n", currentPathSegment->isWildcardPattern);
					fprintf(stderr, "\tprefix=%.*s\n", currentPathSegment->prefixLength, currentPathSegment->prefixBegin);
					fprintf(stderr, "\tpayload=%.*s\n", currentPathSegment->payloadLength, currentPathSegment->payloadBegin);
				}
			}
		#endif

		const char* nullPointer = NULL;
		if ((context.flags & GLOB_DOOFFS) != 0 && ((context.flags & GLOB_APPEND) == 0 || dynamicArraySize(result) == 0)) {
			globInstance->slotsReservedAtTheBeginning = globInstance->gl_offs;

			for (int i = 0; i < globInstance->gl_offs; i++) {
				if (!dynamicArrayInsertAfterLast(result, &nullPointer)) {
					resultSummary = GLOB_NOSPACE;
					break;
				}
			}
		}

		size_t elementCountBefore = dynamicArraySize(result);
		#ifdef DEBUG_WILDCARD_PATH_PATTERN_MATCHER
			fprintf(stderr, "elementCountBefore=%ld\n", elementCountBefore);
		#endif

		if (resultSummary == 0) {
			resultSummary = breadthFirstSearch(&context, result);
		}

		if (resultSummary == 0 && dynamicArraySize(result) - elementCountBefore == 0) {
			if ((flags & GLOB_NOCHECK) != 0) {
				char* patternCopy = strdup(pattern);
				if (patternCopy != NULL) {
					resultSummary = dynamicArrayInsertAfterLast(result, &patternCopy) ? 0 : GLOB_NOSPACE;

				} else {
					resultSummary = GLOB_NOSPACE;
				}

				if (resultSummary != 0) {
					free(patternCopy);
					dynamicArrayClear(result, true);
				}

			} else {
				resultSummary = GLOB_NOMATCH;
			}
		}

		/* Push the final NULL pointer. */
		if (resultSummary == 0 || resultSummary == GLOB_NOMATCH) {
			if (!dynamicArrayInsertAfterLast(result, &nullPointer)) {
				resultSummary = GLOB_NOSPACE;
			}
		}

		if (resultSummary == 0 || resultSummary == GLOB_NOMATCH) {
			char** array = dynamicArrayGetArray(result);
			int size = dynamicArraySize(result);
			if ((flags & GLOB_NOSORT) == 0) {
				qsort(array + elementCountBefore, size - 1 - elementCountBefore, sizeof(char*),
				  (int (*)(const void*, const void*)) &stringComparator);
			}

			globInstance->gl_pathc = size - 1 - globInstance->slotsReservedAtTheBeginning;
			globInstance->gl_pathv = array;
		}
	}

	if (resultSummary != 0 && resultSummary != GLOB_NOMATCH) {
		wildcardPathPatternMatcherRelease(globInstance);
	}
	dynamicArrayClear(&context.pathSegments, true);
	free(context.stringStreamWriterBuffer);

	return resultSummary;
}

void wildcardPathPatternMatcherRelease(glob_t* globInstance) {
	#ifdef DEBUG_WILDCARD_PATH_PATTERN_MATCHER
		fprintf(stderr, "wildcardPathPatternMatcherRelease %p\n", globInstance);
	#endif

	struct DynamicArray* result = &globInstance->result;
	char** array = dynamicArrayGetArray(result);
	for (int i = globInstance->slotsReservedAtTheBeginning; i < dynamicArraySize(result); i++) {
		free(array[i]);
	}
	dynamicArrayClear(result, true);
	globInstance->slotsReservedAtTheBeginning = 0;
}
