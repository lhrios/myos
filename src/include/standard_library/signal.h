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

#ifndef SIGNAL_H
	#define SIGNAL_H

	#include "standard_library_implementation/signal_constants.h"

	#include <stdbool.h>
	#include <stdint.h>

	#include <sys/types.h>

	typedef int sig_atomic_t;
	typedef uint64_t sigset_t;

	inline __attribute__((always_inline)) int sigemptyset(sigset_t* set) {
		*set = 0;
		return 0;
	}
	inline __attribute__((always_inline)) int sigfillset(sigset_t* set) {
		*set = 0xFFFFFFFFFFFFFFFF;
		return 0;
	}
	inline __attribute__((always_inline)) int sigaddset(sigset_t* set, int signalId) {
		*set |= (1 << signalId);
		return 0;
	}
	inline __attribute__((always_inline)) int sigdelset(sigset_t* set, int signalId) {
		*set &= ~(1 << signalId);
		return 0;
	}
	inline __attribute__((always_inline)) int sigismember(const sigset_t* set, int signalId) {
		return *set & (1 << signalId) ? 1 : 0;
	}
	inline __attribute__((always_inline)) sigset_t sigcombine(const sigset_t* set1, const sigset_t* set2) {
		return *set1 | *set2;
	}
	inline __attribute__((always_inline)) sigset_t sigdelall(const sigset_t* set1, const sigset_t* set2) {
		return *set1 & ~*set2;
	}

	typedef struct {
		int si_signo;
		// TODO: There are other required fields according to POSIX documents.
		bool inResponseToUnrecoverableFault;
	} siginfo_t;

	struct sigaction {
		void (*sa_handler)(int);
		void (*sa_sigaction)(int, siginfo_t*, void*); // TODO: Not supported yet.
		sigset_t sa_mask;
		int sa_flags;
	} __attribute__((packed));
	_Static_assert(sizeof(struct sigaction) % sizeof(uint32_t) == 0, "Expecting 4-byte aligned sigaction.");

	#ifndef KERNEL_CODE
		extern const char * const sys_siglist[];
		extern const int sys_nsig;

      typedef void (*sighandler_t)(int);
      sighandler_t signal(int signalId, sighandler_t handler);

		int sigaction(int signalId, const struct sigaction* act, struct sigaction* oldact);
		int sigsuspend(const sigset_t* mask); // TODO: Implement me!
		int __attribute__ ((cdecl)) sigprocmask(int how, const sigset_t* set, sigset_t* oldset);

		int kill(pid_t processId, int signalId);
		int killpg(int processGroupId, int signalId);
		int raise(int signalId);
	#endif

#endif
