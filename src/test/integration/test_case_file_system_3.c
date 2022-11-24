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
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include <myos.h>

#include "util/math_utils.h"

#include "test/integration_test.h"

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	int availableFileDescriptor = OPEN_MAX;
	for (int fileDescriptorIndex = 0; fileDescriptorIndex < OPEN_MAX; fileDescriptorIndex++) {
		struct stat statInstance;
		int result = fstat(fileDescriptorIndex, &statInstance);
		if (result == 0) {
			availableFileDescriptor--;
		}
	}

	bool* releaseFileDescriptor = malloc(OPEN_MAX * sizeof(bool));
	memset(releaseFileDescriptor, 0, OPEN_MAX * sizeof(bool));

	for (int i = 0; i < availableFileDescriptor; i++) {
		int result = open("/dev/null", O_WRONLY);
		assert(result >= 0);
		assert(!releaseFileDescriptor[result]);
		releaseFileDescriptor[result] = true;
	}

	{
		int result = open("/dev/null", O_WRONLY);
		assert(result == -1);
		assert(errno == EMFILE);
	}


	for (int i = 0; i < OPEN_MAX; i++) {
		if (releaseFileDescriptor[i]) {
			close(i);
		}
	}

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
