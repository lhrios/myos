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

BITS 32

section .text

global beforeCallSignalAction

extern callSignalAction
extern callSignalAction

beforeCallSignalAction:
pushfd
pushad

; The stack after executing the previous instruction:
; EIP
; struct sigaction                         -> [ESP + 52]
; sizeof(struct sigaction)                 -> [ESP + 48]
; blockedSignalsForSignalDelivery          -> [ESP + 44]
; blockedSignalsForSignalDelivery          -> [ESP + 40]
; signalId                                 -> [ESP + 36]
; EFLAGS                                   -> [ESP + 32]
; EAX                                      -> [ESP + 28]
; ECX                                      -> [ESP + 24]
; EDX                                      -> [ESP + 20]
; EBX                                      -> [ESP + 16]
; ESP                                      -> [ESP + 12]
; EBP                                      -> [ESP + 08]
; ESI                                      -> [ESP + 04]
; EDI                                      -> [ESP + 00]

lea eax, [esp + 40]
push eax

lea eax, [esp + 52 + 4]
push eax

mov eax, [esp + 36 + 8]
push eax

call callSignalAction

add esp, 12

popad
popfd

add esp, [esp + 12]
add esp, 16

ret
