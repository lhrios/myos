; Copyright 2022 Luis Henrique O. Rios
;
; This file is part of MyOS.
;
; MyOS is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; MyOS is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with MyOS. If not, see <http://www.gnu.org/licenses/>.

%include "x86.asm"

BITS 32
SECTION .data

global interruptionHandlerEntryPointAdresses
global callInterruptionManagerRunCommandsAfterInterruptionHandler

extern x86InterruptionHasErrorCode
extern interruptionManagerHandler
extern interruptionManagerSystemCallHandler
extern interruptionManagerInterruptionVectorToHandleSystemCall
extern interruptionManagerRunCommandsAfterInterruptionHandler

%macro generateInterruptionHandlerEntryPointAddress 1
   dd interruptionHandlerEntryPoint%1
%endmacro

%assign i 0
interruptionHandlerEntryPointAdresses:
%rep 256
   generateInterruptionHandlerEntryPointAddress i
   %assign i i + 1
%endrep

SECTION .text
%macro generateInterruptionHandlerEntryPoint 1
   interruptionHandlerEntryPoint%1:
   push dword 0
   push dword %1
   jmp localInterruptionHandler
%endmacro

%assign i 0
%rep 256
   generateInterruptionHandlerEntryPoint i
   %assign i i + 1
%endrep

localInterruptionHandler:
; The interruptions will be disable here:
; "The only difference between an interrupt gate and a trap gate is the way the processor handles
; the IF flag in the EFLAGS register. When accessing an exception- or interrupt-handling proce-
; dure through an interrupt gate, the processor clears the IF flag to prevent other interrupts from
; interfering with the current interrupt handler. A subsequent IRET instruction restores the IF flag
; to its value in the saved contents of the EFLAGS register on the stack. Accessing a handler
; procedure through a trap gate does not affect the IF flag."

; Save the original values.
pushad
push ds
push es
push fs
push gs

; It adjusts the segment selectors.
mov ax, KERNEL_LINEAR_DATA_SEGMENT_SELECTOR
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax

; The stack after executing the previous instruction:
; SS (if from ring 3)     -> [ESP + 76]
; ESP (if from ring 3)    -> [ESP + 72]
; EFLAGS                  -> [ESP + 68]
; CS                      -> [ESP + 64]
; EIP                     -> [ESP + 60]
; Error code (if any)     -> [ESP + 56]
; Is there an error code? -> [ESP + 52]
; Interruption vector     -> [ESP + 48]
; EAX                     -> [ESP + 44]
; ECX                     -> [ESP + 40]
; EDX                     -> [ESP + 36]
; EBX                     -> [ESP + 32]
; ESP                     -> [ESP + 28]
; EBP                     -> [ESP + 24]
; ESI                     -> [ESP + 20]
; EDI                     -> [ESP + 16]
; DS                      -> [ESP + 12]
; ES                      -> [ESP + 08]
; FS                      -> [ESP + 04]
; GS                      -> [ESP + 00]

; Push ProcessExecutionState2 pointer.
lea eax, [esp + 12]
push eax

; Read the configuration associated with this interruption.
mov eax, [esp + 48 + 4]
cmp eax, [interruptionManagerInterruptionVectorToHandleSystemCall]; Is it a system call?
je interruptionIsSystemCall

push eax
call x86InterruptionHasErrorCode
add esp, 4

cmp eax, 0

je interruptionHasNoErrorCode
   ; Register that there is a error code.
   mov dword [esp + 52 + 4], 1

   ; Push ProcessExecutionState1 pointer.
   lea eax, [esp + 60 + 4]
   push eax
   mov eax, [esp + 56 + 8]
   push eax
   jmp afterInterruptionHasNoErrorCode

interruptionHasNoErrorCode:
   ; Push ProcessExecutionState1 pointer.
   lea eax, [esp + 60 - 4 + 4]
   push eax
   push dword 0 ; Push a dummy error code.

afterInterruptionHasNoErrorCode:

call interruptionManagerHandler
add esp, 12
jmp afterInterruptionIsSystemCall

interruptionIsSystemCall:
   ; Push ProcessExecutionState1 pointer.
   lea eax, [esp + 60 - 4 + 4]
   push eax

   call interruptionManagerSystemCallHandler
   add esp, 8

afterInterruptionIsSystemCall:

callInterruptionManagerRunCommandsAfterInterruptionHandler:
call interruptionManagerRunCommandsAfterInterruptionHandler

pop gs
pop fs
pop es
pop ds
popa

; The stack after executing the previous instruction:
; SS (if from ring 3)     -> [ESP + 28]
; ESP (if from ring 3)    -> [ESP + 24]
; EFLAGS                  -> [ESP + 20]
; CS                      -> [ESP + 16]
; EIP                     -> [ESP + 12]
; Error code (if any)     -> [ESP + 08]
; Is there an error code? -> [ESP + 04]
; Interruption vector     -> [ESP + 00]

cmp dword [esp + 4], 0
je afterAlsoPopErrorCode
alsoPopErrorCode:
	add esp, 4

afterAlsoPopErrorCode:
add esp, 8

iret
