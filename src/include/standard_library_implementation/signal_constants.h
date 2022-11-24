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

#ifndef SIGNAL_CONSTANTS_H
	#define SIGNAL_CONSTANTS_H

	/* The number also represents the priority to handle the signal. Higher number have more priority than lower ones. */
	#define NUMBER_OF_SIGNALS 28
	#define NSIG (NUMBER_OF_SIGNALS + 1)
	#define SIGUSR1 1
	#define SIGUSR2 2
	#define SIGCHLD 3
	#define SIGALRM 4
	#define SIGVTALRM 5
	#define SIGPROF 6
	#define SIGWINCH 7
	#define SIGURG 8
	#define SIGHUP 9
	#define SIGTTOU 10
	#define SIGTTIN 11
	#define SIGTERM 12
	#define SIGQUIT 13
	#define SIGABRT 14
	#define SIGPIPE 15
	#define SIGXFSZ 16
	#define SIGXCPU 17
	#define SIGSYS 18
	#define SIGTRAP 19
	#define SIGINT 20
	#define SIGFPE 21
	#define SIGBUS 22
	#define SIGILL 23
	#define SIGSEGV 24
	#define SIGCONT 25
	#define SIGTSTP 26
	#define SIGSTOP 27
	#define SIGKILL 28

	#define SIG_BLOCK 1
	#define SIG_UNBLOCK 2
	#define SIG_SETMASK 3

	#define SIG_DFL ((void(*)(int)) 1)
	#define SIG_IGN ((void(*)(int)) 2)
	#define SIG_ERR ((void(*)(int)) 3)

	#define SA_NODEFER (1 << 0)
	#define SA_RESTART (1 << 1) // TODO: Implement me! Blocking functions would return a value to let caller know it need to retry the call...
	#define SA_NOCLDSTOP (1 << 2)
	#define SA_RESETHAND (1 << 3) // TODO: Implement me!

#endif
