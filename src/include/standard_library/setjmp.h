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

#ifndef SET_JMP_H
	#define SET_JMP_H

	#include <signal.h>

	struct LongJumpContext {
		uint32_t esp; // 0
		uint32_t ebp; // 4
		uint32_t eip;  // 8
		int restoreSignalsBlockage; // 12
		sigset_t newBlockedSignalsSet; // 16

		uint32_t ebx; // 24
		uint32_t ecx; // 28
		uint32_t edx; // 32
		uint32_t esi; // 36
		uint32_t edi; // 40

		uint32_t eflags; // 44
	} __attribute__((packed));

	typedef struct {
		struct LongJumpContext longJumpContext;
	} __attribute__((packed)) jmp_buf[1];

	typedef struct {
		struct LongJumpContext longJumpContext;
	} __attribute__((packed)) sigjmp_buf[1];

	int __attribute__ ((cdecl)) internalCreateLongJumpContext(void* context, int saveSignalsBlockage);
	void __attribute__ ((cdecl)) internalLongJump(void* context, int value);

	#define setjmp(context) internalCreateLongJumpContext(context, 0)
	#define longjmp(context, value) internalLongJump(context, value)

	#define sigsetjmp(context, saveSignalsBlockage) internalCreateLongJumpContext(context, saveSignalsBlockage)
	#define siglongjmp(context, value) internalLongJump(context, value)

#endif
