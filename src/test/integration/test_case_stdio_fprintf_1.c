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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include "test/integration_test.h"

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	sigemptyset(&act.sa_mask);
	act.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &act, NULL);

	int fd[2];
	int result = pipe(fd);
	assert(result == 0);

	FILE* file = fdopen(fd[1], "w");
	assert(file);

	result = setvbuf(file, NULL, _IONBF, 0);
	assert(result == 0);

	result = fprintf(file, "%d", 123);
	assert(result == 3);
	assert(!ferror(file));
	assert(!feof(file));

	close(fd[0]);

	result = fprintf(file, "%d", 456);
	assert(result == -1);
	assert(ferror(file));
	assert(!feof(file));


	fclose(file);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
