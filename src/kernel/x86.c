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

#include "kernel/x86.h"

const char* const X86_INTERRUPTIONS_NAMES[] = {
	"Divide Error", /* 0 */
	"Reserved", /* 1 */
	"NMI Interrupt", /* 2 */
	"Breakpoint", /* 3 */
	"Overflow", /* 4 */
	"BOUND Range Exceeded", /* 5 */
	"Invalid Opcode (Undefined Opcode)", /* 6 */
	"Device Not Available (No Math Coprocessor)", /* 7 */
	"Double Fault", /* 8 */
	"Reserved", /* 9 */
	"Invalid TSS", /* 10 */
	"Segment Not Present", /* 11 */
	"Stack-Segment Fault", /* 12 */
	"General Protection", /* 13 */
	"Page Fault", /* 14 */
	"Reserved", /* 15 */
	"x87 FPU Floating-PointError (Math Fault)", /* 16 */
	"Alignment Check", /* 17 */
	"Machine Check", /* 18 */
	"SIMD Floating-Point Exception", /* 19 */
	"Reserved", /* 20 */
	"Reserved", /* 21 */
	"Reserved", /* 22 */
	"Reserved", /* 23 */
	"Reserved", /* 24 */
	"Reserved", /* 25 */
	"Reserved", /* 26 */
	"Reserved", /* 27 */
	"Reserved", /* 28 */
	"Reserved", /* 29 */
	"Reserved", /* 30 */
	"Reserved", /* 31 */
};

bool __attribute__ ((cdecl)) x86InterruptionHasErrorCode(uint8_t interruptionVector) {
	switch(interruptionVector) {
		case X86_DOUBLE_FAULT_INTERRUPT_VECTOR:
		case X86_INVALID_TSS_INTERRUPT_VECTOR:
		case X86_SEGMENT_NOT_PRESENT_INTERRUPT_VECTOR:
		case X86_STACK_SEGMENT_FAULT_INTERRUPT_VECTOR:
		case X86_GENERAL_PROTECTION_INTERRUPT_VECTOR:
		case X86_PAGE_FAULT_INTERRUPT_VECTOR:
		case X86_ALIGNMENT_CHECK_INTERRUPT_VECTOR:
			return true;

		default:
			return false;
	}
}

/* GDT and LDT. */
extern uint64_t gdt[];

uint64_t x86ReadFromGDT(int index) {
	return gdt[index];
}

void x86WriteOnGDT(int index, uint64_t segmentDescriptor) {
	gdt[index] = segmentDescriptor;
}

void x86WriteOnLDT(uint64_t* ldt, int index, uint64_t segmentDescriptor) {
	ldt[index] = segmentDescriptor;
}

/* IDT. */
struct X86InterruptGateDescriptor {
	uint16_t offset15_0;
	uint16_t segmentSelector;
	uint8_t always0;
	uint8_t flags;
	uint16_t offset31_16;
} __attribute__((packed));
_Static_assert(sizeof(struct X86InterruptGateDescriptor) == 8, "Expecting X86InterruptGateDescriptor with 8 bytes.");

struct X86IDTLimitAndAddress {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));
_Static_assert(sizeof(struct X86IDTLimitAndAddress) == 6, "Expecting X86IDTLimitAndAddress with 8 bytes.");

static struct X86InterruptGateDescriptor __attribute__((aligned(sizeof(struct X86InterruptGateDescriptor))))
	idt[X86_INTERRUPT_VECTOR_COUNT];
static struct X86IDTLimitAndAddress x86IDTLimitAndAddress;
extern uint32_t interruptionHandlerEntryPointAdresses[];

void x86InitializeIDT(uint8_t interruptionVectorToHandleSystemCall) {
	x86IDTLimitAndAddress.limit = sizeof(idt) - 1;
	x86IDTLimitAndAddress.base = (uint32_t) &idt;

	for (int i = 0; i < X86_INTERRUPT_VECTOR_COUNT; i++) {
		idt[i].offset15_0 = (uint16_t) (interruptionHandlerEntryPointAdresses[i] & 0xFFFF);
		idt[i].segmentSelector = x86SegmentSelector(SYSTEM_KERNEL_LINEAR_CODE_SEGMENT_DESCRIPTOR_INDEX, false, 0);
		idt[i].always0 = 0;
		idt[i].flags = SEGMENT_DESCRIPTOR_PRESENT | SEGMENT_DESCRIPTOR_TYPE_32_BIT_INTERRUPT_GATE;
		idt[i].offset31_16 = (uint16_t) (interruptionHandlerEntryPointAdresses[i] >> 16);
	}

	idt[interruptionVectorToHandleSystemCall].flags = SEGMENT_DESCRIPTOR_PRESENT | SEGMENT_DESCRIPTOR_DPL_3 |
		SEGMENT_DESCRIPTOR_TYPE_32_BIT_INTERRUPT_GATE;

	__asm__ __volatile__(
		"lidt x86IDTLimitAndAddress"
		:
		: "m"(x86IDTLimitAndAddress)
		: "memory");
}
