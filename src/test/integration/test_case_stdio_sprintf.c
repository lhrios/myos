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

#include "util/debug_utils.h"

#include "test/integration_test.h"

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	const size_t bufferSize = 1024;
	char* buffer = malloc(sizeof(char) * bufferSize);
	assert(buffer != NULL);

	const char* string;
	int result;

	string = "ABC 123 ABC     1234\xFF\n";
	result = sprintf(buffer, "%s %d %X %8.d\xFF\n", "ABC", 123, 0xABC, 1234);
	assert(result == strlen(string));
	assert(strcmp(buffer, string) == 0);

	string = "ABC\xFF 123 DEF \xEE\xFF\n\n";
	result = snprintf(buffer, strlen(string), "%s %d %X %c\xFF\n\n", "ABC\xFF", 123, 0xDEF, 0xEE);
	assert(result == strlen(string));
	assert(strncmp(buffer, string, strlen(string) - 1) == 0);

	memset(buffer, 0xFF, bufferSize);
	string = "";
	result = snprintf(buffer, 1, "\n%s\n", "12345678");
	assert(result == 10);
	assert(strncmp(buffer, string, 1) == 0);
	assert(buffer[1] == (char) 0xFF);
	assert(buffer[2] == (char) 0xFF);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
