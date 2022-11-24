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
#include <stdint.h>
#include <stdlib.h>

#include "kernel/interruption_manager.h"
#include "kernel/log.h"
#include "kernel/pic.h"
#include "kernel/x86.h"

/*
 * References:
 * - 8259A Programmable Interrupt Controller (8259A/8259A-2)
 */

#define PIC1_DATA_BUS 0x20 /* I/O base address for master PIC. */
#define PIC2_DATA_BUS 0xA0 /* I/O base address for slave PIC. */
#define PIC1_DATA_BUS_A0_0 PIC1_DATA_BUS
#define PIC1_DATA_BUS_A0_1 (PIC1_DATA_BUS + 1)
#define PIC2_DATA_BUS_A0_0 PIC2_DATA_BUS
#define PIC2_DATA_BUS_A0_1 (PIC2_DATA_BUS + 1)

#define PIC_EOI 0x20 /* End-of-interrupt command code */

static uint8_t masterFirstInterruptionVector;
static uint8_t slaveFirstInterruptionVector;

/*
 * Interrupt Mask Register (IMR).
 */
static uint16_t imrCache;

/*
 * Its necessary to give the PIC some time to react to commands as they might not be processed quickly.
 * It seems to be necessary only during initialization.
 *
 * References:
 * - https://wiki.osdev.org/8259_PIC
 * - https://github.com/torvalds/linux/blob/v4.2/arch/x86/boot/boot.h#L78
 * - https://github.com/torvalds/linux/blob/3c073991eb417b6f785ddc6afbbdc369eb84aa6a/arch/x86/kernel/i8259.c
 */
static inline __attribute__((always_inline)) void ioWait(int iterations){
	/* Port 0x80 is used for 'checkpoints' during POST. */
	/* The Linux kernel seems to think it is free for use :-/ */

	for (int c = 0; c < iterations; c++) {
		x86OutputByteToPort(0x80, 0);
	}
}

static void issueEndOfIRQ7(void) {
	picIssueEndOfInterrupt(IRQ7, true);
}

static void handleIRQ7(uint32_t errorCode, struct ProcessExecutionState1* processExecutionState1, struct ProcessExecutionState2* processExecutionState2) {
	assert(picIsSpuriousIRQ(IRQ7));
	logDebug("An spurious IRQ just happened (IRQ7)");
	interruptionManagerRegisterCommandToRunAfterInterruptionHandler(PRIORITY_LOWEST, (void(*)(void*)) &issueEndOfIRQ7, NULL);
}

static void issueEndOfIRQ15(void) {
	picIssueEndOfInterrupt(IRQ15, true);
}

static void handleIRQ15(uint32_t errorCode, struct ProcessExecutionState1* processExecutionState1, struct ProcessExecutionState2* processExecutionState2) {
	assert(picIsSpuriousIRQ(IRQ15));
	logDebug("An spurious IRQ just happened (IRQ15)");
	interruptionManagerRegisterCommandToRunAfterInterruptionHandler(PRIORITY_LOWEST, (void(*)(void*)) &issueEndOfIRQ15, NULL);
}

void picInitialize(uint8_t newMasterFirstInterruptionVector, uint8_t newSlaveFirstInterruptionVector) {
	masterFirstInterruptionVector = newMasterFirstInterruptionVector;
	slaveFirstInterruptionVector = newSlaveFirstInterruptionVector;

	interruptionManagerRegisterInterruptionHandler(masterFirstInterruptionVector + 7, &handleIRQ7);
	interruptionManagerRegisterInterruptionHandler(slaveFirstInterruptionVector + 7, &handleIRQ15);
}

void picInitializeHardware(void) {
	picDisableIRQs(ALL_IRQs);
	ioWait(2);

	/* ICW1. */
	x86OutputByteToPort(PIC1_DATA_BUS_A0_0, 0x11); /* Master: ICW4 needed | D4 == 1 | cascade mode | edge triggered mode. */
	ioWait(2);
	x86OutputByteToPort(PIC2_DATA_BUS_A0_0, 0x11); /* Slave. */
	ioWait(2);

	/* ICW2. */
	x86OutputByteToPort(PIC1_DATA_BUS_A0_1, masterFirstInterruptionVector);
	ioWait(2);
	x86OutputByteToPort(PIC2_DATA_BUS_A0_1, slaveFirstInterruptionVector);
	ioWait(2);

	/* ICW3. */
	x86OutputByteToPort(PIC1_DATA_BUS_A0_1, 0x04); /* Master: slave at IRQ2. */
	ioWait(2);
	x86OutputByteToPort(PIC2_DATA_BUS_A0_1, 0x02); /* Slave: slave connected at master's IRQ2 (its id must be 2). */
	ioWait(2);

	/* ICW4. */
	x86OutputByteToPort(PIC1_DATA_BUS_A0_1, 0x01); /* Master: 8086/8088 mode | normal EOI (not automatic)
		| not special fully nested mode | non buffered mode. */
	ioWait(2);
	x86OutputByteToPort(PIC2_DATA_BUS_A0_1, 0x01); /* Slave. */
	ioWait(100);

	/*
	 * The PIC has an internal register called the IMR, or the Interrupt Mask Register.
	 * It is 8 bits wide. This register is a bitmap of the request lines going into the PIC.
	 * When a bit is set, the PIC ignores the request and continues normal operation.
	 */
	picDisableIRQs(ALL_IRQs);
	ioWait(2);

	/*
	 * After initialization the 8259A is set to IRR.
	 */
}

void picDisableIRQs(uint16_t irqs) {
	imrCache = imrCache | irqs;

	/* OCW1. */
	x86OutputByteToPort(PIC1_DATA_BUS_A0_1, (imrCache & 0xFF));
	x86OutputByteToPort(PIC2_DATA_BUS_A0_1, (imrCache >> 8));
}

void picEnableIRQs(uint16_t irqs) {
	imrCache = (~irqs) & imrCache;

	/* OCW1. */
	x86OutputByteToPort(PIC1_DATA_BUS_A0_1, (imrCache & 0xFF));
	x86OutputByteToPort(PIC2_DATA_BUS_A0_1, (imrCache >> 8));
}

bool picIsSpuriousIRQ(uint16_t irq) {
	bool isCertainlySpurious = irq & imrCache;
	if (!isCertainlySpurious) {
		uint16_t isr; /* In-Service Register. */

		if (irq <= IRQ7) {
			/* As the 8259A "remembers" whether the IRR or ISR has been previously selected by the OCW3. */
			x86OutputByteToPort(PIC1_DATA_BUS_A0_0, 0x0B); /* OCW3: no action | read ISR | no poll */
			/* OCW1. */
			isr = x86InputByteFromPort(PIC1_DATA_BUS_A0_0);

		} else {
			/* As the 8259A "remembers" whether the IRR or ISR has been previously selected by the OCW3. */
			x86OutputByteToPort(PIC2_DATA_BUS_A0_0, 0x0B); /* OCW3: no action | read ISR | no poll */
			/* OCW1. */
			isr = x86InputByteFromPort(PIC2_DATA_BUS_A0_0) << 8;
		}

		return !(isr & irq);

	} else {
		return true;
	}
}

void picIssueEndOfInterrupt(uint16_t irq, bool isSpurious) {
	if (!isSpurious || irq > IRQ7) {
		x86OutputByteToPort(PIC1_DATA_BUS_A0_0, 0x20); /* non-specific EOI command. */
	}

	if (!isSpurious && irq > IRQ7) {
		x86OutputByteToPort(PIC2_DATA_BUS_A0_0, 0x20); /* non-specific EOI command. */
	}
}
