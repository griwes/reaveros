/*
 * Copyright © 2021-2022 Michał 'Griwes' Dominiak
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

.global load_gdtr
.global load_idtr
.global interrupt_handler_stub
.global syscall_handler_stub

.global interrupt_ret

.extern interrupt_handler
.extern syscall_handler

load_gdtr:
    lgdt [rdi]

    push 0x8
    lea rax, [rip + _ret]
    push rax

    retfq

_ret:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov ax, 0x28
    ltr ax

    ret

load_idtr:
    lidt [rdi]

    ret

interrupt_handler_stub:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    call interrupt_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16

interrupt_ret:
    iretq

syscall_handler_stub:
    swapgs

    mov rdx, rsp
    mov rsp, gs:[0]
    mov rsp, [rsp]

    // create space on the stack for a potential iretq frame
    push 0x23
    push 0
    push 0
    push 0x1b
    push 0

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    call syscall_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    swapgs

    cmp qword ptr [rsp], 0
    jne _sysret_use_iretq

    mov rsp, rdx

    sysretq

_sysret_use_iretq:
    iretq
