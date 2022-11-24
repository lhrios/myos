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

section .text

global internalCreateLongJumpContext
global internalLongJump

extern sigprocmask

internalCreateLongJumpContext:
	mov eax, [esp + 4] ; context

	mov [eax + 24], ebx
	mov [eax + 28], ecx
	mov [eax + 32], edx
	mov [eax + 36], esi
	mov [eax + 40], edi

	pushfd
	pop ebx
	mov [eax + 44], ebx

	mov [eax + 0], esp
	mov [eax + 4], ebp

	mov ecx, [esp]
	mov [eax + 8], ecx

	mov [eax + 12], dword 0

	mov ecx, [esp + 8] ; save signals blockage?

	cmp ecx, 0
	jz .doNotSaveSignalsBlockage
	mov [eax + 12], dword 1

	push eax

	add eax, 16
	push eax
	push 0 ; NULL
	push 3 ; SIG_SETMASK
	call sigprocmask

	add esp, 12
	pop eax
	.doNotSaveSignalsBlockage:

	mov ebx, [eax + 24]
	mov ecx, [eax + 28]

	mov eax, 0
	ret

internalLongJump:
	mov eax, [esp + 4] ; context
	mov ecx, [esp + 8] ; value

	; Default value if it is zero.
	cmp ecx, 0
	jne .isNotZero
	mov ecx, 1
	.isNotZero:

	mov esp, [eax + 0]
	mov ebp, [eax + 4]
	mov edx, [eax + 8]
	mov [esp], edx

	push ecx

	mov ecx, [eax + 12]
	cmp ecx, 0
	jz .doNotRestoreSignalsBlockage

	push eax

	push 0 ; NULL
	mov ecx, eax
	add ecx, 16
	push ecx
	push 3 ; SIG_SETMASK
	call sigprocmask
	add esp, 12

	pop eax

	.doNotRestoreSignalsBlockage:

	mov ecx, [eax + 44]
	push ecx
	popfd

	mov ebx, [eax + 24]
	mov ecx, [eax + 28]
	mov edx, [eax + 32]
	mov esi, [eax + 36]
	mov edi, [eax + 40]

	pop eax

	ret
