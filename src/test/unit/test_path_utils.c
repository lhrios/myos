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
#include <stdio.h>
#include <string.h>

#include "kernel/limits.h"

#include "util/path_utils.h"

static void testPathUtilsCalculatePathSegments(void) {
	struct PathUtilsContext* pathUtilsContext = malloc(sizeof(struct PathUtilsContext));
	APIStatusCode result;

	strcpy(pathUtilsContext->buffer, "/home/luis/gitrepos/UVaOnline.git/.../a.txt");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 6);
	assert(strcmp(pathUtilsContext->segments[0], "home") == 0);
	assert(strcmp(pathUtilsContext->segments[1], "luis") == 0);
	assert(strcmp(pathUtilsContext->segments[2], "gitrepos") == 0);
	assert(strcmp(pathUtilsContext->segments[3], "UVaOnline.git") == 0);
	assert(strcmp(pathUtilsContext->segments[4], "...") == 0);
	assert(strcmp(pathUtilsContext->segments[5], "a.txt") == 0);

	strcpy(pathUtilsContext->buffer, "/");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 0);

	strcpy(pathUtilsContext->buffer, "/opt/tools/bin");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 3);
	assert(strcmp(pathUtilsContext->segments[0], "opt") == 0);
	assert(strcmp(pathUtilsContext->segments[1], "tools") == 0);
	assert(strcmp(pathUtilsContext->segments[2], "bin") == 0);

	strcpy(pathUtilsContext->buffer, "/opt/./../tools/.././/////bin");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 7);
	assert(strcmp(pathUtilsContext->segments[0], "opt") == 0);
	assert(strcmp(pathUtilsContext->segments[1], ".") == 0);
	assert(strcmp(pathUtilsContext->segments[2], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[3], "tools") == 0);
	assert(strcmp(pathUtilsContext->segments[4], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[5], ".") == 0);
	assert(strcmp(pathUtilsContext->segments[6], "bin") == 0);

	memset(pathUtilsContext->buffer, 'a', FILE_NAME_MAX_LENGTH - 1);
	pathUtilsContext->buffer[FILE_NAME_MAX_LENGTH - 1] = '\0';
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 1);

	memset(pathUtilsContext->buffer, 'a', FILE_NAME_MAX_LENGTH);
	pathUtilsContext->buffer[FILE_NAME_MAX_LENGTH] = '\0';
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == ENAMETOOLONG);

	free(pathUtilsContext);
}

static void testPathUtilsConcatenate(void) {
	struct PathUtilsContext* pathUtilsContext = malloc(sizeof(struct PathUtilsContext));
	APIStatusCode result;

	result = pathUtilsConcatenate(pathUtilsContext, "/home/luis/", "/test/hello_world", -1, -1);
	assert(result == SUCCESS);
	assert(strcmp(pathUtilsContext->buffer, "/home/luis/test/hello_world") == 0);

	result = pathUtilsConcatenate(pathUtilsContext, "", "/bin/test/hello_world", -1, -1);
	assert(result == SUCCESS);
	assert(strcmp(pathUtilsContext->buffer, "/bin/test/hello_world") == 0);

	result = pathUtilsConcatenate(pathUtilsContext, "", "", -1, -1);
	assert(result == SUCCESS);
	assert(strcmp(pathUtilsContext->buffer, "/") == 0);

	result = pathUtilsConcatenate(pathUtilsContext, "abc", "", 0, -1);
	assert(result == SUCCESS);
	assert(strcmp(pathUtilsContext->buffer, "/") == 0);

	free(pathUtilsContext);
}

static void testPathUtilsNormalize(void) {
	struct PathUtilsContext* pathUtilsContext = malloc(sizeof(struct PathUtilsContext));
	APIStatusCode result;

	strcpy(pathUtilsContext->buffer, "/opt/./../tools/.././/////bin");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 7);
	assert(strcmp(pathUtilsContext->segments[0], "opt") == 0);
	assert(strcmp(pathUtilsContext->segments[1], ".") == 0);
	assert(strcmp(pathUtilsContext->segments[2], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[3], "tools") == 0);
	assert(strcmp(pathUtilsContext->segments[4], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[5], ".") == 0);
	assert(strcmp(pathUtilsContext->segments[6], "bin") == 0);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, true);
	assert(strcmp(pathUtilsContext->buffer, "/bin") == 0);

	strcpy(pathUtilsContext->buffer, "/home/luis/programacao/eclipse_workspaces/workspace_myos/MyOS/src/kernel/../../target/bin/standard_library/time.o");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 14);
	assert(strcmp(pathUtilsContext->segments[0], "home") == 0);
	assert(strcmp(pathUtilsContext->segments[1], "luis") == 0);
	assert(strcmp(pathUtilsContext->segments[2], "programacao") == 0);
	assert(strcmp(pathUtilsContext->segments[3], "eclipse_workspaces") == 0);
	assert(strcmp(pathUtilsContext->segments[4], "workspace_myos") == 0);
	assert(strcmp(pathUtilsContext->segments[5], "MyOS") == 0);
	assert(strcmp(pathUtilsContext->segments[6], "src") == 0);
	assert(strcmp(pathUtilsContext->segments[7], "kernel") == 0);
	assert(strcmp(pathUtilsContext->segments[8], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[9], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[10], "target") == 0);
	assert(strcmp(pathUtilsContext->segments[11], "bin") == 0);
	assert(strcmp(pathUtilsContext->segments[12], "standard_library") == 0);
	assert(strcmp(pathUtilsContext->segments[13], "time.o") == 0);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, true);
	assert(strcmp(pathUtilsContext->buffer, "/home/luis/programacao/eclipse_workspaces/workspace_myos/MyOS/target/bin/standard_library/time.o") == 0);

	strcpy(pathUtilsContext->buffer, "/usr/lib/firefox/distribution/extensions/../../defaults/../../../games/flare");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 13);
	assert(strcmp(pathUtilsContext->segments[0], "usr") == 0);
	assert(strcmp(pathUtilsContext->segments[1], "lib") == 0);
	assert(strcmp(pathUtilsContext->segments[2], "firefox") == 0);
	assert(strcmp(pathUtilsContext->segments[3], "distribution") == 0);
	assert(strcmp(pathUtilsContext->segments[4], "extensions") == 0);
	assert(strcmp(pathUtilsContext->segments[5], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[6], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[7], "defaults") == 0);
	assert(strcmp(pathUtilsContext->segments[8], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[9], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[10], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[11], "games") == 0);
	assert(strcmp(pathUtilsContext->segments[12], "flare") == 0);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, true);
	assert(strcmp(pathUtilsContext->buffer, "/usr/games/flare") == 0);

	strcpy(pathUtilsContext->buffer, "/../../tmp/../../../tmp/output");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 8);
	assert(strcmp(pathUtilsContext->segments[0], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[1], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[2], "tmp") == 0);
	assert(strcmp(pathUtilsContext->segments[3], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[4], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[5], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[6], "tmp") == 0);
	assert(strcmp(pathUtilsContext->segments[7], "output") == 0);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, true);
	assert(strcmp(pathUtilsContext->buffer, "/tmp/output") == 0);

	strcpy(pathUtilsContext->buffer, "/");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 0);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, true);
	assert(strcmp(pathUtilsContext->buffer, "/") == 0);

	strcpy(pathUtilsContext->buffer, "/tmp/../abc/my_file.txt");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 4);
	assert(strcmp(pathUtilsContext->segments[0], "tmp") == 0);
	assert(strcmp(pathUtilsContext->segments[1], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[2], "abc") == 0);
	assert(strcmp(pathUtilsContext->segments[3], "my_file.txt") == 0);
	pathUtilsNormalize(pathUtilsContext);
	assert(pathUtilsContext->segmentCount == 2);
	assert(strcmp(pathUtilsContext->segments[0], "abc") == 0);
	assert(strcmp(pathUtilsContext->segments[1], "my_file.txt") == 0);
	pathUtilsCombinePathSegments(pathUtilsContext, false);
	assert(strcmp(pathUtilsContext->buffer, "/abc") == 0);
	assert(pathUtilsContext->hasLastSegment);
	assert(strcmp(pathUtilsContext->lastSegment, "my_file.txt") == 0);

	strcpy(pathUtilsContext->buffer, "/");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 0);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, false);
	assert(strcmp(pathUtilsContext->buffer, "/") == 0);
	assert(!pathUtilsContext->hasLastSegment);

	strcpy(pathUtilsContext->buffer, "/../../");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 2);
	assert(strcmp(pathUtilsContext->segments[0], "..") == 0);
	assert(strcmp(pathUtilsContext->segments[1], "..") == 0);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, false);
	assert(strcmp(pathUtilsContext->buffer, "/") == 0);
	assert(!pathUtilsContext->hasLastSegment);

	strcpy(pathUtilsContext->buffer, "/tmp/z/x/./abc/");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 5);
	assert(strcmp(pathUtilsContext->segments[0], "tmp") == 0);
	assert(strcmp(pathUtilsContext->segments[1], "z") == 0);
	assert(strcmp(pathUtilsContext->segments[2], "x") == 0);
	assert(strcmp(pathUtilsContext->segments[3], ".") == 0);
	assert(strcmp(pathUtilsContext->segments[4], "abc") == 0);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, false);
	assert(strcmp(pathUtilsContext->buffer, "/tmp/z/x") == 0);
	assert(pathUtilsContext->hasLastSegment);
	assert(strcmp(pathUtilsContext->lastSegment, "abc") == 0);

	strcpy(pathUtilsContext->buffer, "/444");
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 1);
	assert(strcmp(pathUtilsContext->segments[0], "444") == 0);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, false);
	assert(strcmp(pathUtilsContext->buffer, "/") == 0);
	assert(pathUtilsContext->hasLastSegment);
	assert(strcmp(pathUtilsContext->lastSegment, "444") == 0);

	const char* veryBigPath = "/tmp/aaa/aaaaaaaaaaaaaaaa/bbbb/ccccccccccc/eeeeeeeeeeeee/aaaaaaaaa/zzzzzzzzzzzzzzzz/eeeeeeeeeeeee/rrrrrrrrrrr/hhhh"
			"/88888888888/333333333/11111111/2222222222/77777777777/XXXXXXXXXX/yyyyyyyyyyyyyyyyyyy/oooooooooooooooooo/pppppppppppppp/qqqqqqqqqqqqqqqqq/rrrrrrrrrr/ssssssssssss"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/hhhfffffffffffffffffffffffffff/ffff";
	assert(strlen(veryBigPath) + 1 == PATH_MAX_LENGTH);

	strcpy(pathUtilsContext->buffer, veryBigPath);
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 86);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, false);
	assert(pathUtilsContext->segmentCount == 85);
	assert(strncmp(pathUtilsContext->buffer, veryBigPath, PATH_MAX_LENGTH - strlen("ffff") - 2) == 0);
	assert(pathUtilsContext->hasLastSegment);
	assert(strcmp(pathUtilsContext->lastSegment, "ffff") == 0);

	veryBigPath = "/tmp/aaa/aaaaaaaaaaaaaaaa/bbbb/ccccccccccc/eeeeeeeeeeeee/aaaaaaaaa/zzzzzzzzzzzzzzzz/eeeeeeeeeeeee/rrrrrrrrrrr/hhhh"
			"/88888888888/333333333/11111111/2222222222/77777777777/XXXXXXXXXX/yyyyyyyyyyyyyyyyyyy/oooooooooooooooooo/pppppppppppppp/qqqqqqqqqqqqqqqqq/rrrrrrrrrr/ssssssssssss"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/ppppppppppppppppppppppppppppppppppppppppp/jjjjjjjjjjjjjjjjjjjjjjjjjjj/xxxxxxxxxxxxxxxxxxxxxxx/iiiiiiiiiiiiiii"
			"/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/912ec803b2ce49e4a541068d495ab5703/.";
	assert(strlen(veryBigPath) + 1 == PATH_MAX_LENGTH);

	strcpy(pathUtilsContext->buffer, veryBigPath);
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 86);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, false);
	assert(pathUtilsContext->segmentCount == 84);
	assert(strncmp(pathUtilsContext->buffer, veryBigPath, PATH_MAX_LENGTH - strlen("912ec803b2ce49e4a541068d495ab5703") - 2 - 2) == 0);
	assert(pathUtilsContext->hasLastSegment);
	assert(strcmp(pathUtilsContext->lastSegment, "912ec803b2ce49e4a541068d495ab5703") == 0);

	veryBigPath = "aaaaaaaaaaaaaaaa3bb3ccccccccaedddddddddddddddddddddddddddddddddddddddddddeeeeeeaaa3zzzzzzzzzzzzxeeeeeeeeeecrrrrrrrrrchhh"
			"a8888888888b333333333s11111111s22222c7777ddd7dddddddddd7sXXXcyyyyyyyyyyyyyyyyyycooooooooooooooooocppppppppcqqqqqqqqqqcrrrrrrcssssssssss";
	assert(strlen(veryBigPath) + 1 == FILE_NAME_MAX_LENGTH);

	strcpy(pathUtilsContext->buffer, veryBigPath);
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == SUCCESS);
	assert(pathUtilsContext->segmentCount == 1);
	pathUtilsNormalize(pathUtilsContext);
	pathUtilsCombinePathSegments(pathUtilsContext, false);
	assert(pathUtilsContext->segmentCount == 1);
	assert(strcmp(pathUtilsContext->buffer, "/") == 0);
	assert(pathUtilsContext->hasLastSegment);
	assert(strcmp(pathUtilsContext->lastSegment, veryBigPath) == 0);

	veryBigPath = "/a/aaaaaaaaaaaaaaaa3bb3ccccccccaedddddddddddddddddddddddddddddddddddddddddddeeeeeeaaa3zzzzzzzzzzzzxeeeeeeeeeecrrrrrrrrrchhh"
			"a8888888888b333333333s11111111s22222c7777ddd7dddddddddd7sXXXcyyyyyyyyyyyyyyyyyycooooooooooooooooocppppppppcqqqqqqqqqqcrrrrrrcssssssssss2";
	assert(strlen(veryBigPath) == 3 + FILE_NAME_MAX_LENGTH);

	strcpy(pathUtilsContext->buffer, veryBigPath);
	result = pathUtilsCalculatePathSegments(pathUtilsContext);
	assert(result == ENAMETOOLONG);

	free(pathUtilsContext);
}

static void testPathUtilsFindLastPathSegment(void) {
	const char* path;
	const char* lastPathSegment;
	size_t pathLength;
	size_t segmentLength;

	path = "/test/usr/bin";
	pathLength = strlen(path);
	lastPathSegment = pathUtilsFindLastPathSegment(path, pathLength, &segmentLength);
	assert(strcmp(lastPathSegment, "bin") == 0);
	assert(3 == segmentLength);

	path = "/test";
	pathLength = strlen(path);
	lastPathSegment = pathUtilsFindLastPathSegment(path, pathLength, &segmentLength);
	assert(strcmp(lastPathSegment, "test") == 0);
	assert(4 == segmentLength);

	path = "/";
	pathLength = strlen(path);
	lastPathSegment = pathUtilsFindLastPathSegment(path, pathLength, &segmentLength);
	assert(strcmp(lastPathSegment, "") == 0);
	assert(0 == segmentLength);
}

static void testPathUtilsParsePath(void) {
	struct PathUtilsContext* pathUtilsContext = malloc(sizeof(struct PathUtilsContext));
	APIStatusCode result;
	const char* path;
	const char* currentWorkingDirectory;
	size_t currentWorkingDirectoryLength;

	path = "/test";
	currentWorkingDirectory = "/";
	currentWorkingDirectoryLength = strlen(currentWorkingDirectory);
	result = pathUtilsParsePath(pathUtilsContext,
			path, true, true,
			currentWorkingDirectory, currentWorkingDirectoryLength);
	assert(strcmp(pathUtilsContext->buffer, path) == 0);
	assert(!pathUtilsContext->hasLastSegment);
	assert(result == SUCCESS);

	path = "/a/test";
	currentWorkingDirectory = "/";
	currentWorkingDirectoryLength = strlen(currentWorkingDirectory);
	result = pathUtilsParsePath(pathUtilsContext,
			path, true, false,
			currentWorkingDirectory, currentWorkingDirectoryLength);
	assert(strcmp(pathUtilsContext->buffer, "/a") == 0);
	assert(pathUtilsContext->hasLastSegment);
	assert(strcmp(pathUtilsContext->lastSegment, "test") == 0);
	assert(result == SUCCESS);

	path = "/";
	currentWorkingDirectory = "/";
	currentWorkingDirectoryLength = strlen(currentWorkingDirectory);
	result = pathUtilsParsePath(pathUtilsContext,
			path, true, false,
			currentWorkingDirectory, currentWorkingDirectoryLength);
	assert(strcmp(pathUtilsContext->buffer, "/") == 0);
	assert(!pathUtilsContext->hasLastSegment);
	assert(result == SUCCESS);

	path = "abc/def";
	currentWorkingDirectory = "/";
	currentWorkingDirectoryLength = strlen(currentWorkingDirectory);
	result = pathUtilsParsePath(pathUtilsContext,
			path, false, false,
			currentWorkingDirectory, currentWorkingDirectoryLength);
	assert(strcmp(pathUtilsContext->buffer, "/abc") == 0);
	assert(pathUtilsContext->hasLastSegment);
	assert(strcmp(pathUtilsContext->lastSegment, "def") == 0);
	assert(result == SUCCESS);

	path = "def/efg/./././.";
	currentWorkingDirectory = "/abc";
	currentWorkingDirectoryLength = strlen(currentWorkingDirectory);
	result = pathUtilsParsePath(pathUtilsContext,
			path, false, false,
			currentWorkingDirectory, currentWorkingDirectoryLength);
	assert(strcmp(pathUtilsContext->buffer, "/abc/def") == 0);
	assert(pathUtilsContext->hasLastSegment);
	assert(strcmp(pathUtilsContext->lastSegment, "efg") == 0);
	assert(result == SUCCESS);

	path = "../../a/b/c/d/";
	currentWorkingDirectory = "/0/1";
	currentWorkingDirectoryLength = strlen(currentWorkingDirectory);
	result = pathUtilsParsePath(pathUtilsContext,
			path, false, false,
			currentWorkingDirectory, currentWorkingDirectoryLength);
	assert(strcmp(pathUtilsContext->buffer, "/a/b/c") == 0);
	assert(pathUtilsContext->hasLastSegment);
	assert(strcmp(pathUtilsContext->lastSegment, "d") == 0);
	assert(result == SUCCESS);
}

int main(void) {
	testPathUtilsCalculatePathSegments();
	testPathUtilsConcatenate();
	testPathUtilsNormalize();
	testPathUtilsFindLastPathSegment();
	testPathUtilsParsePath();

	return 0;
}
