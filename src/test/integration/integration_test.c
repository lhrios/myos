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

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <myos.h>

#include "util/string_stream_writer.h"

#include "test/integration_test.h"

static void handleSignal(int signalId) {
	fprintf(stderr, "Will terminate due to signal %s\n", strsignal(signalId));
	raise(SIGKILL);
}

void integrationTestConfigureCommonSignalHandlers(void) {
	struct sigaction action;
	action.sa_handler = &handleSignal;
	action.sa_sigaction = NULL;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	int result;
	result = sigaction(SIGSEGV, &action, NULL);
	assert(result == 0);

	result = sigaction(SIGILL, &action, NULL);
	assert(result == 0);
}

void integrationTestRegisterSuccessfulCompletion(const char* testCaseName) {
	char buffer[PATH_MAX_LENGTH];
	struct StringStreamWriter stringStreamWriter;

	stringStreamWriterInitialize(&stringStreamWriter, buffer, PATH_MAX_LENGTH);
	streamWriterWriteString(&stringStreamWriter.streamWriter, INTEGRATION_TEST_OUTPUT_BASE_PATH, UINT_MAX);
	streamWriterWriteString(&stringStreamWriter.streamWriter, "/", UINT_MAX);
	streamWriterWriteString(&stringStreamWriter.streamWriter, testCaseName, UINT_MAX);
	streamWriterWriteString(&stringStreamWriter.streamWriter, ".succeeded", UINT_MAX);
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);

	FILE* file = fopen(buffer, "w+");
	assert(file != NULL);
	fclose(file);
}

static int nextTemporaryFileNameSuffix = 0;

char* integrationTestCreateTemporaryFileName(const char* prefix) {
	char* result = malloc(strlen(prefix) + 1 + 8 + 1 + 3 + 1);
	if (result != NULL) {
		sprintf(result, "%s_%08x.tmp", prefix, nextTemporaryFileNameSuffix++);
	}
	return result;
}

bool integrationTestFileExists(const char* fileName) {
    int fileDescriptorIndex = open(fileName, O_RDONLY);
    if (fileDescriptorIndex >= 0) {
   	 close(fileDescriptorIndex);
   	 return true;
    } else {
   	 return false;
    }
}

char* integrationReadFileContentAsString(const char* fileName) {
	FILE* file = fopen(fileName, "r");
	assert(file != NULL);
	int result = fseek(file, 0, SEEK_END);
	assert(result == 0);
	int fileSize = ftell(file);
	rewind(file);

	char* content = malloc(sizeof(char) * (fileSize + 1));
	fread(content, fileSize, 1, file);
	content[fileSize] = '\0';
	fclose(file);

	return content;
}
