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

.global load_gdt

load_gdt:
    lea rax, [rip + _gdt_base]
    lea rcx, [rip + _gdt_start]
    mov qword ptr [rax], rcx

    lea rax, [rip + _gdt]
    lgdt [rax]

    push 0x8
    lea rax, [rip + _new_gdt]
    push rax

    retfq

_new_gdt:

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret

_gdt_start:
    ; null:
    .8byte 0

    ; code:
    .2byte 0
    .2byte 0
    .byte 0
    .byte 0b10011010
    .byte 0b00100000
    .byte 0

    ; data:
    .2byte 0
    .2byte 0
    .byte 0
    .byte 0b10010010
    .byte 0
    .byte 0

_gdt_end:

_gdt:
    .2byte _gdt_end - _gdt_start - 1
_gdt_base:
    .8byte 0

