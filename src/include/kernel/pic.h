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

#ifndef KERNEL_PIC_H
	#define KERNEL_PIC_H

	#include <stdbool.h>
	#include <stdint.h>

	#define ALL_IRQs 0xFFFF

	enum IRQ {
		IRQ0 = 0x0001,
		IRQ1 = 0x0002,
		IRQ2 = 0x0004,
		IRQ3 = 0x0008,
		IRQ4 = 0x0010,
		IRQ5 = 0x0020,
		IRQ6 = 0x0040,
		IRQ7 = 0x0080,

		IRQ8 = 0x0100,
		IRQ9 = 0x0200 ,
		IRQ10 = 0x0400,
		IRQ11 = 0x0800,
		IRQ12 = 0x1000,
		IRQ13 = 0x2000,
		IRQ14 = 0x4000,
		IRQ15 = 0x8000,
	};

	void picInitialize(uint8_t masterFirstInterruptionVector, uint8_t slaveFirstInterruptionVector);
	void picInitializeHardware(void);
	void picDisableIRQs(uint16_t irqs);
	void picEnableIRQs(uint16_t irqs);
	bool picIsSpuriousIRQ(uint16_t irq);
	void picIssueEndOfInterrupt(uint16_t irq, bool isSpurious);
#endif
