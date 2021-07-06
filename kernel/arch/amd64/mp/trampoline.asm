/*
 * Copyright © 2021 Michał 'Griwes' Dominiak
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

.global trampoline_start
.global trampoline_end
.global trampoline_asid_slot
.global trampoline_stack_slot
.global trampoline_flag_slot

.extern ap_initialize

.set _gdt_offset, _gdt_start - trampoline_start
.set _gdtr_offset, _gdtr - trampoline_start
.set _pmode_offset, _pmode - trampoline_start
.set _lmode_offset, _lmode - trampoline_start
.set _stack_top_offset, _stack_top - trampoline_start
.set _asid_slot_offset, trampoline_asid_slot - trampoline_start
.set _flag_slot_offset, trampoline_flag_slot - trampoline_start
.set _stack_slot_offset, trampoline_stack_slot - trampoline_start

.code16

trampoline_start:
    inc byte ptr cs:[_flag_slot_offset]

    cli

    mov ax, cs
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    lea sp, [_stack_top_offset]

    mov eax, cs
    mov ebx, 0x10
    mul ebx

    mov esi, eax

    lea ebx, [eax + _gdt_offset]
    mov dword ptr cs:[_gdtr_offset + 2], ebx

    lgdt cs:[_gdtr_offset]

    lea eax, [esi + _pmode_offset]

    push 0x8
    push eax

    mov eax, cr0
    or al, 1
    mov cr0, eax

    retf

.code32

_pmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    lea esp, [esi + _stack_top_offset]

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, [esi + _asid_slot_offset]
    mov cr3, eax

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    push 0x18
    lea eax, [esi + _lmode_offset]
    push eax

    retf

.code64

_lmode:
    mov rbp, 0
    mov rsp, [esi + _stack_slot_offset]

    inc byte ptr [esi + _flag_slot_offset]

    lea rax, [ap_initialize]
    jmp rax

.fill 4096, 1, 0
_stack_top:

trampoline_flag_slot:
    .byte 0
trampoline_asid_slot:
    .8byte 0
trampoline_stack_slot:
    .8byte 0

_gdt_start:
    // null - 0x0
    .4byte 0
    .4byte 0

    // code - 0x8
    .2byte 0xffff
    .2byte 0
    .byte 0
    .byte 0b10011010
    .byte 0b11001111
    .byte 0

    // data - 0x10
    .2byte 0xffff
    .2byte 0
    .byte 0
    .byte 0b10010010
    .byte 0b11001111
    .byte 0

    // code 64 bit - 0x18
    .2byte 0
    .2byte 0
    .byte 0
    .byte 0b10011000
    .byte 0b00100000
    .byte 0

    // data 64 bit - 0x20
    .2byte 0
    .2byte 0
    .byte 0
    .byte 0b10010000
    .byte 0
    .byte 0

_gdt_end:

_gdtr:
    .2byte 39
    .4byte 0

trampoline_end:
