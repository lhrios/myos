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
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "test/integration_test.h"

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	{
		struct sigaction action;
		int result = sigaction(SIGINT, NULL, &action);
		assert(result == 0);
		assert(action.sa_handler == SIG_DFL);
	}

	{
		struct sigaction action;
		memset(&action, 0, sizeof(struct sigaction));
		action.sa_handler = SIG_IGN;
		int result = sigaction(SIGCHLD, &action, NULL);
		assert(result == 0);

		memset(&action, 0, sizeof(struct sigaction));
		action.sa_handler = SIG_IGN;
		result = sigaction(SIGCHLD, NULL, &action);
		assert(result == 0);
		assert(action.sa_handler == SIG_IGN);
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
