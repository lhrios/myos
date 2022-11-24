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
#include <strings.h>

#include "test/integration_test.h"

static void testFfs(void) {
	int result;

	result = ffs(0x10);
	assert(result == 5);

	result = ffs(0x80000000);
	assert(result == 32);

	result = ffs(0x8000);
	assert(result == 16);

	result = ffs(0x1);
	assert(result == 1);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	testFfs();

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
