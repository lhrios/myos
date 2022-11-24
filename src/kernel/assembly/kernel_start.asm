; Copyright 2022 Luis Henrique O. Rios
;
; This file is part of MyOS kernel.
;
; MyOS kernel is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; MyOS kernel is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with MyOS. If not, see <http://www.gnu.org/licenses/>.

BITS 32

; After initialization, this value will be employed as stack for system's x86 task.
global INITIALIZATION_STACK_TOP
section .bss
	resb 4096; It must have at least the size of a page frame (4Kb).
	INITIALIZATION_STACK_TOP:

section .text
; This module is the first piece of code executed when the kernel starts.
; The reason of most of the things done here are described in the Multiboot Specification
; ("http://www.gnu.org/software/grub/manual/multiboot/multiboot.html"). So, the kernel must
; be loaded by a Multiboot conformant loader (like GRUB).

extern MyOSMain
start:
	lgdt [gdtLimitAndAddress] ; It loads the GDT.
	mov cx, (KERNEL_LINEAR_DATA_SEGMENT_DESCRIPTOR_INDEX * 8)
	mov ss, cx
	mov ds, cx
	mov es, cx
	mov fs, cx
	mov gs, cx
	jmp (KERNEL_LINEAR_CODE_SEGMENT_DESCRIPTOR_INDEX * 8):.afterLoadGDT ; It runs the code using the new code segment.

.afterLoadGDT:
	mov esp, INITIALIZATION_STACK_TOP ; It prepares the stack.

	push ebx
	push eax

	push 0x2 ; The eflags register must be initialized. The following flags will be set:
	popf	 ; 1 : (mandatory)

; IA-32 Intel® Architecture Software Developer's Manual Volume 3: System Programming Guide
; 10.5.3. Preventing Caching
; 	To disable the L1, L2, and L3 caches after they have been enabled and have received cache fills, perform the following steps:
;		1. Enter the no-fill cache mode. (Set the CD flag in control register CR0 to 1 and the NW flag to 0.
;		2. Flush all caches using the WBINVD instruction.
;		3. Disable the MTRRs and set the default memory type to uncached or set all MTRRs for the uncached memory type (see the discussion
;			of the discussion of the TYPE field and the Eflag in Section 10.11.2.1., “IA32_MTRR_DEF_TYPE MSR”).
	mov eax, 0x40000033
	mov cr0, eax ; The cr0 register must be initialized too. The following flags will be set:
				 ; Protection Enable (bit 0 of CR0): true
				 ; Monitor Coprocessor (bit 1 of CR0): true
				 ; Emulation (bit 2 of CR0): false
				 ; Task Switched (bit 3 of CR0): false
				 ; Extension Type (bit 4 of CR0): true
				 ; Numeric Error (bit 5 of CR0): true
				 ; Write Protect (bit 16 of CR0): false
				 ; Alignment Mask (bit 18 of CR0): false
				 ; Not Write-through (bit 29 of CR0): false
				 ; Cache Disable (bit 30 of CR0): true
				 ; Paging (bit 31 of CR0): false
				 ;
				 ; The processor cache system will be enabled using C code.
	wbinvd
	call MyOSMain
	jmp $

; Multiboot header (it needs to be close to the binary begin):
extern DATA_SECTION_END
extern BSS_SECTION_END

%define MULTIBOOT_HEADER_MAGIC 0x1BADB002 ; MULTIBOOT_HEADER_MAGIC
%define MULTIBOOT_HEADER_FLAGS 0x00010006 ; MULTIBOOT_AOUT_KLUDGE | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE

align 4, db 0
header:
	dd MULTIBOOT_HEADER_MAGIC ; magic
	dd MULTIBOOT_HEADER_FLAGS ; flags
	dd 0 - MULTIBOOT_HEADER_MAGIC - MULTIBOOT_HEADER_FLAGS ; checksum
	dd header ; header_addr
	dd start ; load_addr
	dd DATA_SECTION_END ; load_end_addr
	dd BSS_SECTION_END ; bss_end_addr
	dd start ; entry_addr
	dd 1 ; mode_type
	dd 80 ; width
	dd 25 ; height
	dd 0 ; depth

section .data
align 8, db 0

; GDT. It will use the protected flat model.
global gdt
gdt:

;0x0 0 (the first descriptor in GDT is not used):
	dq 0x0

;0x8 1:
KERNEL_LINEAR_DATA_SEGMENT_DESCRIPTOR_INDEX equ ($-gdt) / 8
	dw 0xFFFF ; segment limit (15:0 - 16 bits): 15:0
	dw 0x0000 ; base address (31:16 - 16 bits): 15:0

	db 0x00 ; base address (7:0 - 8 bits): 23:16

	db 0x92 ; flags 1 (15:8 - 8 bits): SEGMENT_PRESENT | DPL_0 | DATA_READ_WRITE
	db 0xCF ; segment limit (19:16 - 4 bits): 19:16
			  ; flags 2 (23:20 - 4 bits):  GRANULARITY_4_KBYTES | 32_BIT_SEGMENT

	db 0x00 ; Base address (31:24 - 8 bits): 31:24

;0x10 2:
KERNEL_LINEAR_CODE_SEGMENT_DESCRIPTOR_INDEX equ ($-gdt) / 8
	dw 0xFFFF ; segment limit (15:0 - 16 bits): 15:0
	dw 0x0000 ; base address (31:16 - 16 bits): 15:0

	db 0x00 ; base address (7:0 - 8 bits): 23:16

	db 0x9A ; flags 1 (15:8 - 8 bits): SEGMENT_PRESENT | DPL_0 | CODE_EXECUTE_READ
	db 0xCF ; segment limit (19:16 - 4 bits): 19:16
			  ; flags 2 (23:20 - 4 bits):  GRANULARITY_4_KBYTES | 32_BIT_SEGMENT

	db 0x00 ; base address (31:24 - 8 bits): 31:24

;0x18 3: It will be used to store a Task State segment descriptor.
		;There will be no more than one TSS on GDT at any given time.
	dq 0x0

;0x20 4:
USER_LINEAR_DATA_SEGMENT_DESCRIPTOR_INDEX equ ($-gdt) / 8
	dw 0xFFFF ; segment limit (15:0 - 16 bits): 15:0
	dw 0x0000 ; base address (31:16 - 16 bits): 15:0

	db 0x00 ; base address (7:0 - 8 bits): 23:16

	db 0xF2 ; flags 1 (15:8 - 8 bits): SEGMENT_PRESENT | DPL_3 | DATA_READ_WRITE
	db 0xCF ; segment limit (19:16 - 4 bits): 19:16
			  ; flags 2 (23:20 - 4 bits):  GRANULARITY_4_KBYTES | 32_BIT_SEGMENT

	db 0x00 ; base address (31:24 - 8 bits): 31:24

;0x28 5:
USER_LINEAR_CODE_SEGMENT_DESCRIPTOR_INDEX equ ($-gdt) / 8
	dw 0xFFFF ; segment limit (15:0 - 16 bits): 15:0
	dw 0x0000 ; base address (31:16 - 16 bits): 15:0

	db 0x00 ; base address (7:0 - 8 bits): 23:16

	db 0xFA ; flags 1 (15:8 - 8 bits): SEGMENT_PRESENT | DPL_3 | CODE_EXECUTE_READ
	db 0xCF ; segment limit (19:16 - 4 bits): 19:16
			  ; flags 2 (23:20 - 4 bits):  GRANULARITY_4_KBYTES | 32_BIT_SEGMENT

	db 0x00 ; base address (31:24 - 8 bits): 31:24

gdtEnd:

gdtLimitAndAddress:
	dw gdtEnd - gdt - 1 ; limit
	dd gdt ; base address
