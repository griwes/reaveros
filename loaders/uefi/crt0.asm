/*
 * Copyright © 2023 Michał 'Griwes' Dominiak
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

.extern image_base
.extern _DYNAMIC

.extern relocate
.extern efi_main

.global _start

_start:
    mov rdi, 0x0
    mov dword ptr [rdi], 0xaeaeaeae

    //lea rdi, [rip + image_base]
    //lea rsi, [rip + _DYNAMIC]

    push rcx
    push rdx

    // will start calling this once I really, really need to
    //call relocate

    pop rsi
    pop rdi

    call efi_main

    ret

.section .data

dummy: .4byte 0

.section .reloc, "a"

label1:
    .4byte dummy - label1
    .4byte 10
    .2byte 0

