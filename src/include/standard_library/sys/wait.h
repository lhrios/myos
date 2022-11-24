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

#ifndef SYS_WAIT_H
	#define SYS_WAIT_H

	#include <signal.h>

	#include <sys/types.h>

	#define WIFEXITED_MASK (1<<31)
	#define WIFSIGNALED_MASK (1<<30)
	#define WIFSTOPPED_MASK (1<<29)
	#define WIFCONTINUED_MASK (1<<28)

	#define WIFEXITED(status) ((WIFEXITED_MASK & status) != 0)
	#define WIFSIGNALED(status) ((WIFSIGNALED_MASK & status) != 0)
	#define WIFSTOPPED(status) ((WIFSTOPPED_MASK & status) != 0)
	#define WIFCONTINUED(status) ((WIFCONTINUED_MASK & status) != 0)

	#define WEXITSTATUS_SHIFT 0
	#define WTERMSIG_SHIFT 8
	#define WSTOPSIG_SHIFT 16

	#define WEXITSTATUS_MASK 0xFF
	#define WTERMSIG_MASK 0xFF00
	#define WSTOPSIG_MASK 0xFF0000

	#define WEXITSTATUS(status) ((WEXITSTATUS_MASK & status) >> WEXITSTATUS_SHIFT)
	#define WTERMSIG(status) ((WTERMSIG_MASK & status) >> WTERMSIG_SHIFT)
	#define WSTOPSIG(status) ((WSTOPSIG_MASK & status) >> WSTOPSIG_SHIFT)
	#define WCOREDUMP(status) (0)

	#define WCONTINUED 0x1
	#define WNOHANG 0x2
	#define WUNTRACED 0x4

	pid_t wait(int *wstatus);
	pid_t waitpid(pid_t, int*, int);

#endif
