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
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "test/integration_test.h"

static void testCtime() {
	const time_t timeInstance = 1361060085; /* GMT: Sunday, 17 February 2013 00:14:45 */
   const char* result = ctime(&timeInstance);
	const char* expected = "Sun Feb 17 00:14:45 2013";
	assert(strcmp(expected, result) == 0);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	testCtime();

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
