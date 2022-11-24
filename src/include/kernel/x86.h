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

#ifndef KERNEL_X86_H
	#define KERNEL_X86_H

	#include <assert.h>
	#include <stdint.h>
	#include <stdbool.h>

	/* An easy way to access some registers. */
	inline __attribute__((always_inline)) uint16_t x86GetSS(void) {
		uint16_t result;
		__asm__ __volatile__(
			"mov %%ss, %0"
			: "=r"(result)
			:
			:);
		return result;
	}

	inline __attribute__((always_inline)) uint16_t x86GetCS(void) {
		uint16_t result;
		__asm__ __volatile__(
			"mov %%cs, %0"
			: "=r"(result)
			:
			:);
		return result;
	}

	inline __attribute__((always_inline)) uint32_t x86GetESP(void) {
		uint32_t result;
		__asm__ __volatile__(
			"mov %%esp, %0"
			: "=r"(result)
			:
			:);
		return result;
	}

	inline __attribute__((always_inline)) uint32_t x86GetCR0(void) {
		uint32_t result;
		__asm__ __volatile__(
			"mov %%cr0, %0"
			: "=r"(result)
			:
			:);
		return result;
	}

	inline __attribute__((always_inline)) void x86SetCR0(uint32_t cr0) {
		__asm__ __volatile__(
			"mov %0, %%cr0"
			:
			: "r"(cr0)
			:);
	}

	inline __attribute__((always_inline)) uint32_t x86GetCR2(void) {
		uint32_t result;
		__asm__ __volatile__(
			"mov %%cr2, %0"
			: "=r"(result)
			:
			:);
		return result;
	}

	inline __attribute__((always_inline)) uint32_t x86GetCR3(void) {
		uint32_t result;
		__asm__ __volatile__(
			"mov %%cr3, %0"
			: "=r"(result)
			:
			:);
		return result;
	}

	inline __attribute__((always_inline)) void x86SetCR3(uint32_t cr3) {
		__asm__ __volatile__(
			"mov %0, %%cr3"
			:
			: "r"(cr3)
			:);
	}

	inline __attribute__((always_inline)) uint32_t x86GetCR4(void) {
		uint32_t result;
		__asm__ __volatile__(
			"mov %%cr4, %0"
			: "=r"(result)
			:
			:);
		return result;
	}

	inline __attribute__((always_inline)) void x86SetCR4(uint32_t cr4) {
		__asm__ __volatile__(
			"mov %0, %%cr4"
			:
			: "r"(cr4)
			:);
	}

	inline __attribute__((always_inline)) uint64_t x86GetMSR(uint32_t msrId) {
		uint32_t resultLower;
		uint32_t resultUpper;
		__asm__ __volatile__(
			"rdmsr"
			: "=a"(resultLower), "=d"(resultUpper)
			: "c"(msrId)
			:);
		return (((uint64_t) resultUpper) << 32) | resultLower;
	}

	inline __attribute__((always_inline)) void x86SetMSR(uint32_t msrId, uint64_t content) {
		uint32_t resultLower = (uint32_t) content;
		uint32_t resultUpper = (uint32_t) (content >> 32);
		__asm__ __volatile__(
			"wrmsr"
			:
			: "d"(resultUpper), "a"(resultLower), "c"(msrId)
			:);
	}

	/* Some instructions. */
	inline __attribute__((always_inline)) void x86Hlt(void) {
		__asm__ __volatile__(
			"hlt"
			:
			:
			:);
	}

	inline __attribute__((always_inline)) void x86Cli(void) {
		__asm__ __volatile__(
			"cli"
			:
			:
			: "cc");
	}

	inline __attribute__((always_inline)) uint32_t x86GetEflags(void) {
		uint32_t eflags;
		__asm__ __volatile__(
			"pushf;"
			"pop %%eax;"
			:"=a"(eflags)
			:
			:);
		return eflags;
	}

	inline __attribute__((always_inline)) void x86Sti(void) {
		__asm__ __volatile__(
			"sti"
			:
			:
			: "cc");
	}

	inline __attribute__((always_inline)) uint64_t x86Rdtsc(void) {
		uint32_t resultLower;
		uint32_t resultUpper;
		__asm__ __volatile__(
			"rdtsc"
			: "=a"(resultLower), "=d"(resultUpper)
			:
			:);
		return (((uint64_t) resultUpper) << 32) | resultLower;
	}

	inline __attribute__((always_inline)) void x86Ring0Stop(void) {
		while (true) {
			x86Hlt();
		}
	}

	inline __attribute__((always_inline)) void x86InvalidateTLBEntry(uint32_t address) {
		__asm__ __volatile__(
			"invlpg (%0)"
			:
			: "r"(address)
			:);
	}

	/*
	 * I/O.
	 */
	inline __attribute__((always_inline)) uint8_t x86InputByteFromPort(uint16_t port) {
		uint8_t result;

		__asm__ __volatile__(
			"inb %%dx, %%al"
			: "=a"(result)
			: "d"(port)
			: "memory");
		return result;
	}

	inline __attribute__((always_inline)) void x86OutputByteToPort(uint16_t port, uint8_t value) {
		__asm__ __volatile__(
			"outb %%al, %%dx"
			:
			: "a"(value), "d"(port)
			: "memory");
	}

	inline __attribute__((always_inline)) uint16_t x86InputWordFromPort(uint16_t port) {
		uint16_t result;

		__asm__ __volatile__(
			"inw %%dx, %%ax"
			: "=a"(result)
			: "d"(port)
			: "memory");
		return result;
	}

	inline __attribute__((always_inline)) void x86OutpuWordFromPort(uint16_t port, uint16_t value) {
		__asm__ __volatile__(
			"outw %%ax, %%dx"
			:
			: "a"(value), "d"(port)
			: "memory");
	}

	/*
	 * Interruptions.
	 */
	#define X86_DIVIDE_ERROR_INTERRUPT_VECTOR 0
	#define X86_INVALID_OPCODE_INTERRUPT_VECTOR 6
	#define X86_DEVICE_NOT_AVAILABLE_NO_MATH_COPROCESSOR 7
	#define X86_DOUBLE_FAULT_INTERRUPT_VECTOR 8
	#define X86_INVALID_TSS_INTERRUPT_VECTOR 10
	#define X86_SEGMENT_NOT_PRESENT_INTERRUPT_VECTOR 11
	#define X86_STACK_SEGMENT_FAULT_INTERRUPT_VECTOR 12
	#define X86_GENERAL_PROTECTION_INTERRUPT_VECTOR 13
	#define X86_PAGE_FAULT_INTERRUPT_VECTOR 14
	#define X86_ALIGNMENT_CHECK_INTERRUPT_VECTOR 17
	#define X86_FIRST_NON_RESERVED_INTERRUPT_VECTOR 32

	#define X86_INTERRUPT_VECTOR_COUNT 256

	extern const char* const X86_INTERRUPTIONS_NAMES[];
	void x86InitializeIDT(uint8_t interruptionVectorToHandleSystemCall);
	bool __attribute__ ((cdecl)) x86InterruptionHasErrorCode(uint8_t interruptionVector);

	/*
	 * GDT.
	 */
	#define SYSTEM_KERNEL_LINEAR_DATA_SEGMENT_DESCRIPTOR_INDEX 1
	#define SYSTEM_KERNEL_LINEAR_CODE_SEGMENT_DESCRIPTOR_INDEX 2
	#define TASK_STATE_SEGMENT_DESCRIPTOR_INDEX 3
	#define LDT_DESCRIPTOR_INDEX 4
	uint64_t x86ReadFromGDT(int index);
	void x86WriteOnGDT(int index, uint64_t segmentDescriptor);

	/*
	 * X86 structures.
	 */
	struct X86TaskState {
		uint16_t previousTaskLink;
		uint16_t reserved0;

		uint32_t esp0;
		uint16_t ss0;
		uint16_t reserved1;

		uint32_t esp1;
		uint16_t ss1;
		uint16_t reserved2;

		uint32_t esp2;
		uint16_t ss2;
		uint16_t reserved3;

		uint32_t cr3;
		uint32_t eip;
		uint32_t eflags;
		uint32_t eax;
		uint32_t ecx;
		uint32_t edx;
		uint32_t ebx;
		uint32_t esp;
		uint32_t ebp;
		uint32_t esi;
		uint32_t edi;

		uint16_t es;
		uint16_t reserved4;
		uint16_t cs;
		uint16_t reserved5;
		uint16_t ss;
		uint16_t reserved6;
		uint16_t ds;
		uint16_t reserved7;
		uint16_t fs;
		uint16_t reserved8;
		uint16_t gs;
		uint16_t reserved9;

		uint16_t ldt;
		uint16_t reserved10;

		uint16_t debugTrapAndReserved11;
		uint16_t ioMapBaseAddress;
	} __attribute__((packed));
	_Static_assert(sizeof(struct X86TaskState) == 104, "Expecting X86TaskState with 104 bytes.");

	/* Constants related with paging. */
	#define PAGE_DIRECTORY_LENGTH 1024
	#define PAGE_TABLE_LENGTH 1024

	#define PAGE_ENTRY_PRESENT 0x1
	#define PAGE_ENTRY_NOT_PRESENT 0x0

	#define PAGE_ENTRY_READ_ONLY  0x0
	#define PAGE_ENTRY_READ_WRITE 0x2

	#define PAGE_ENTRY_SYSTEM 0x0
	#define PAGE_ENTRY_USER 0x4

	#define PAGE_ENTRY_WRITE_THROUGH 0x8

	#define PAGE_ENTRY_CACHE_ENABLED 0x00
	#define PAGE_ENTRY_CACHE_DISABLED 0x10

	#define PAGE_ENTRY_ACCESSED 0x20

	#define PAGE_ENTRY_DIRTY 0x40

	#define PAGE_ENTRY_SIZE_4_MBYTES 0x80
	#define PAGE_ENTRY_SIZE_4_KBYTES 0x0

	#define PAGE_ENTRY_GLOBAL 0x100
	#define PAGE_ENTRY_LOCAL  0x0

	#define PAGE_ENTRY_PAGE_TABLE_ATTRIBUTE_INDEX 0x80

	#define PAGE_FAULT_EXCEPTION_NON_PRESENT_PAGE 0x0
	#define PAGE_FAULT_EXCEPTION_PAGE_LEVEL_PROTECTION_VIOLATION 0x1

	#define PAGE_FAULT_EXCEPTION_READ 0x0
	#define PAGE_FAULT_EXCEPTION_WRITE 0x2

	#define PAGE_FAULT_EXCEPTION_SYSTEM_MODE 0x0
	#define PAGE_FAULT_EXCEPTION_USER_MODE 0x4

	#define PAGE_FAULT_EXCEPTION_WAS_NOT_RESERVED_BIT_VIOLATION 0x0
	#define PAGE_FAULT_EXCEPTION_WAS_RESERVED_BIT_VIOLATION 0x8

	/*
	 * Constants related with segment selectors.
	 */
	#define SEGMENT_SELECTOR_RLP_MASK 0x3

	/*
	 * Constants related with segment descriptors.
	 */
	#define SEGMENT_DESCRIPTOR_SIZE 8

	#define SEGMENT_DESCRIPTOR_PRESENT 0x80
	#define SEGMENT_DESCRIPTOR_NOT_PRESENT 0x00

	#define SEGMENT_DESCRIPTOR_DPL_0 0x00
	#define SEGMENT_DESCRIPTOR_DPL_1 0x20
	#define SEGMENT_DESCRIPTOR_DPL_2 0x40
	#define SEGMENT_DESCRIPTOR_DPL_3 0x60

	#define SEGMENT_DESCRIPTOR_TYPE_CODE_EXECUTE_READ 0x1A
	#define SEGMENT_DESCRIPTOR_TYPE_DATA_READ_WRITE 0x12

	#define SEGMENT_DESCRIPTOR_TYPE_LDT 0x02
	#define SEGMENT_DESCRIPTOR_TYPE_32_BIT_TSS_AVAILABLE 0x09
	#define SEGMENT_DESCRIPTOR_TYPE_32_BIT_INTERRUPT_GATE 0x0E

	#define SEGMENT_DESCRIPTOR_16_BIT 0x0000
	#define SEGMENT_DESCRIPTOR_32_BIT 0x4000

	#define SEGMENT_DESCRIPTOR_GRANULARITY_1_BYTE 0x0000
	#define SEGMENT_DESCRIPTOR_GRANULARITY_4_KBYTES 0x8000

	/*
	 * Constants related with EFLAGS.
	 */
	#define EFLAGS_IDENTIFICATION_FLAG_MASK 0x200000
	#define EFLAGS_VIRTUAL_INTERRUPT_PENDING_MASK 0x100000
	#define EFLAGS_VIRTUAL_INTERRUPT_FLAG_MASK 0x80000
	#define EFLAGS_ALIGNMENT_CHECK_MASK 0x40000
	#define EFLAGS_VIRTUAL_8086_MODE_MASK 0x20000
	#define EFLAGS_RESUME_FLAG_MASK 0x10000
	#define EFLAGS_NESTED_TASK_FLAG_MASK 0x4000
	#define EFLAGS_IO PRIVILEGE_LEVEL_MASK 0x3000
	#define EFLAGS_OVERFLOW_FLAG_MASK  0x800
	#define EFLAGS_DIRECTION_FLAG_MASK 0x400
	#define EFLAGS_INTERRUPT_ENABLE_FLAG_MASK 0x200
	#define EFLAGS_TRAP_FLAG_MASK 0x100
	#define EFLAGS_SIGN_FLAG_MASK 0x80
	#define EFLAGS_ZERO_FLAG_MASK 0x40
	#define EFLAGS_AUXILIARY_CARRY_FLAG_MASK 0x10
	#define EFLAGS_PARITY_FLAG_MASK 0x4
	#define EFLAGS_CARRY_FLAG_MASK 0x1
	#define EFLAGS_RESERVED 0x2

	inline __attribute__((always_inline)) uint16_t x86SegmentSelector(uint16_t index, bool ldt, uint32_t rpl) {
		return (index << 3) | ((ldt ? 1 : 0) << 2) | (rpl & SEGMENT_SELECTOR_RLP_MASK);
	}

	inline __attribute__((always_inline)) uint64_t x86SegmentDescriptor(uint32_t base, uint32_t limit, uint16_t flags) {
		uint64_t segmentDescriptor = 0;

		segmentDescriptor |= (((uint64_t) base) << 16) & 0xFFFFFF0000ULL;
		segmentDescriptor |= ((uint64_t) ((base) & 0xFF000000)) << 32;

		segmentDescriptor |= limit & 0xFFFF;
		segmentDescriptor |= ((uint64_t) ((limit) & 0xF0000)) << 32;

		segmentDescriptor |= ((uint64_t) flags) << 40;

		return segmentDescriptor;
	}

	inline __attribute__((always_inline)) uint8_t x86GetSegmentSelectorRPL(uint16_t segmentSelector) {
		return segmentSelector & SEGMENT_SELECTOR_RLP_MASK;
	}

	/*
	 * Constants related with MSRS (model-specific registers).
	 */
	#define IA32_TIME_STAMP_COUNTER 0x10

	/*
	 * FPU related.
	 */
	#define X86_FPU_STATE_LENGTH 512 /* 16-byte aligned */

	inline __attribute__((always_inline)) void x86Fxsave(void* fpuState) {
		assert(((uint32_t) fpuState) % 16 == 0);
		__asm__ __volatile__(
			"fxsave (%0)"
			:
			: "r"(fpuState)
			: "memory");
	}

	inline __attribute__((always_inline)) void x86Fxrstor(void* fpuState) {
		assert(((uint32_t) fpuState) % 16 == 0);
		__asm__ __volatile__(
			"fxrstor (%0)"
			:
			: "r"(fpuState)
			: "memory");
	}

	inline __attribute__((always_inline)) void x86Fninit(void) {
		__asm__ __volatile__(
			"fninit"
			:
			:
			: "memory");
	}

	inline __attribute__((always_inline)) uint16_t x86Fnstcw(void) {
		uint16_t fpuControlWord;
		__asm__ __volatile__(
			"fnstcw %0"
			: "=m"(fpuControlWord)
			:
			:);
		return fpuControlWord;
	}

	inline __attribute__((always_inline)) void x86ClearTaskSwitchedFlag(void) {
		__asm__ __volatile__(
			"clts"
			:
			:
			:);
	}

#endif
