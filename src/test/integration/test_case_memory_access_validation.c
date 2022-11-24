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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include <sys/stat.h>

#include <myos.h>

#include "test/integration_test.h"

static void testInvalidMemoryAccesses(struct ProcessMemorySegmentLimits* processMemorySegmentLimits) {

	if (!processMemorySegmentLimits->isEmpty) {
			void* firstInvalidAddressBelow = (void*) (processMemorySegmentLimits->firstAddress - 1);
			void* firstInvalidAddressAbove = (void*) (processMemorySegmentLimits->lastAddress);

			int result;
			void* resultAsPointer;

			{
				result = myosGetProcessMemorySegmentsLimits(firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
				result = myosGetProcessMemorySegmentsLimits(firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
			}

			{
				resultAsPointer = opendir(firstInvalidAddressBelow);
				assert(resultAsPointer == NULL && errno == EFAULT);
				resultAsPointer = opendir(firstInvalidAddressAbove);
				assert(resultAsPointer == NULL && errno == EFAULT);
			}

			{
				result = write(STDOUT_FILENO, firstInvalidAddressBelow, 1);
				assert(result != 0 && errno == EFAULT);
				result = write(STDOUT_FILENO, firstInvalidAddressAbove, 1);
				assert(result != 0 && errno == EFAULT);
			}

			{
				int zeroFileDescriptorIndex = open("/dev/zero", O_RDONLY);
				assert(zeroFileDescriptorIndex >= 0);

				result = read(zeroFileDescriptorIndex, firstInvalidAddressBelow, 1);
				assert(result != 0 && errno == EFAULT);
				result = read(zeroFileDescriptorIndex, firstInvalidAddressAbove, 1);
				assert(result != 0 && errno == EFAULT);

				result = close(zeroFileDescriptorIndex);
				assert(result == 0);
			}

			{
				result = chdir(firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
				result = chdir(firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
			}

			{
				result = unlink(firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
				result = unlink(firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
			}

			{
				result = rmdir(firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
				result = rmdir(firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
			}

			{
				result = truncate(firstInvalidAddressBelow, 0);
				assert(result != 0 && errno == EFAULT);
				result = truncate(firstInvalidAddressAbove, 0);
				assert(result != 0 && errno == EFAULT);
			}

			{
				result = execve(firstInvalidAddressBelow, NULL, NULL);
				assert(result != 0 && errno == EFAULT);
				result = execve(firstInvalidAddressAbove, NULL, NULL);
				assert(result != 0 && errno == EFAULT);
			}

			{
				result = execve("/something_that_does_not_exist", firstInvalidAddressBelow, NULL);
				assert(result != 0 && errno == EFAULT);
				result = execve("/something_that_does_not_exist", firstInvalidAddressAbove, NULL);
				assert(result != 0 && errno == EFAULT);
			}

			{
				char* argv[2];
				argv[1] = NULL;

				argv[0] = firstInvalidAddressBelow;
				result = execve("/something_that_does_not_exist", argv, NULL);
				assert(result != 0 && errno == EFAULT);

				argv[0] = firstInvalidAddressAbove;
				result = execve("/something_that_does_not_exist", argv, NULL);
				assert(result != 0 && errno == EFAULT);
			}

			{
				char* envp[2];
				envp[1] = NULL;

				envp[0] = firstInvalidAddressBelow;
				result = execve("/something_that_does_not_exist", NULL, envp);
				assert(result != 0 && errno == EFAULT);

				envp[0] = firstInvalidAddressAbove;
				result = execve("/something_that_does_not_exist", NULL, envp);
				assert(result != 0 && errno == EFAULT);
			}

			{
				int ttyFileDescriptorIndex = open("/dev/tty02", O_RDONLY);
				assert(ttyFileDescriptorIndex >= 0);

				result = tcgetattr(ttyFileDescriptorIndex, firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
				result = tcgetattr(ttyFileDescriptorIndex, firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);

				result = tcsetattr(ttyFileDescriptorIndex, TCSANOW, firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
				result = tcsetattr(ttyFileDescriptorIndex, TCSANOW, firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);

				result = close(ttyFileDescriptorIndex);
				assert(result == 0);
			}

			{
				struct sigaction act;

				result = sigaction(SIGQUIT, firstInvalidAddressAbove, &act);
				assert(result != 0 && errno == EFAULT);
				result = sigaction(SIGQUIT, firstInvalidAddressBelow, &act);
				assert(result != 0 && errno == EFAULT);

				result = sigaction(SIGQUIT, &act, firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
				result = sigaction(SIGQUIT, &act, firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
			}

			{
				struct sigaction act;

				result = sigaction(SIGQUIT, firstInvalidAddressAbove, &act);
				assert(result != 0 && errno == EFAULT);
				result = sigaction(SIGQUIT, firstInvalidAddressBelow, &act);
				assert(result != 0 && errno == EFAULT);

				result = sigaction(SIGQUIT, &act, firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
				result = sigaction(SIGQUIT, &act, firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
			}

			{
				sigset_t set;

				result = sigprocmask(SIG_BLOCK, firstInvalidAddressAbove, &set);
				assert(result != 0 && errno == EFAULT);
				result = sigprocmask(SIG_BLOCK, firstInvalidAddressBelow, &set);
				assert(result != 0 && errno == EFAULT);

				result = sigprocmask(SIG_BLOCK, &set, firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
				result = sigprocmask(SIG_BLOCK, &set, firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
			}

			{
				int ttyFileDescriptorIndex = open("/dev/tty02", O_RDONLY);
				assert(ttyFileDescriptorIndex >= 0);

				result = fstat(ttyFileDescriptorIndex, firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
				result = fstat(ttyFileDescriptorIndex, firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);

				result = close(ttyFileDescriptorIndex);
				assert(result == 0);
			}

			{
				result = open(firstInvalidAddressAbove, O_RDONLY);
				assert(result != 0 && errno == EFAULT);
				result = open(firstInvalidAddressBelow, O_RDONLY);
				assert(result != 0 && errno == EFAULT);
			}

			{
				result = link(firstInvalidAddressAbove, "alias_of_something");
				assert(result != 0 && errno == EFAULT);
				result = link(firstInvalidAddressBelow, "alias_of_something");
				assert(result != 0 && errno == EFAULT);

				result = link("/dev/tty02", firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
				result = link("/dev/tty02", firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
			}

			{
				result = chdir(firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
				result = chdir(firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
			}

			{
				result = rmdir(firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
				result = rmdir(firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
			}

			{
				result = symlink(firstInvalidAddressAbove, "alias_of_something");
				assert(result != 0 && errno == EFAULT);
				result = symlink(firstInvalidAddressBelow, "alias_of_something");
				assert(result != 0 && errno == EFAULT);

				result = symlink("/dev/tty02", firstInvalidAddressAbove);
				assert(result != 0 && errno == EFAULT);
				result = symlink("/dev/tty02", firstInvalidAddressBelow);
				assert(result != 0 && errno == EFAULT);
			}

			{
				const char* result = getcwd(firstInvalidAddressAbove, INT_MAX);
				assert(result == NULL && errno == EFAULT);
				result = getcwd(firstInvalidAddressBelow, INT_MAX);
				assert(result == NULL && errno == EFAULT);
			}

			{
				result = mkdir(firstInvalidAddressAbove, 0);
				assert(result != 0 && errno == EFAULT);
				result = mkdir(firstInvalidAddressBelow, 0);
				assert(result != 0 && errno == EFAULT);
			}

			{
				result = poll(firstInvalidAddressAbove, 1, 0);
				assert(result != 0 && errno == EFAULT);
				result = poll(firstInvalidAddressBelow, 1, 0);
				assert(result != 0 && errno == EFAULT);
			}

			// TODO: struct dirent* readdir(DIR* directory) {
		}
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	struct ProcessMemorySegmentsLimits processMemorySegmentsLimits;
	int result = myosGetProcessMemorySegmentsLimits(&processMemorySegmentsLimits);
	assert(result == 0);

	testInvalidMemoryAccesses(&processMemorySegmentsLimits.code);
	testInvalidMemoryAccesses(&processMemorySegmentsLimits.data);
	testInvalidMemoryAccesses(&processMemorySegmentsLimits.stack);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
