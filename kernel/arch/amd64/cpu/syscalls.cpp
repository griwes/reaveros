/*
 * Copyright © 2022 Michał 'Griwes' Dominiak
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

#include "syscalls.h"

#include <kernel/meta.h>

#include "../../../scheduler/thread.h"
#include "core.h"
#include "cpu.h"

namespace kernel::amd64::syscalls
{
void context::save_to(thread::context * ctx) const
{
    ctx->r15 = r15;
    ctx->r14 = r14;
    ctx->r13 = r13;
    ctx->r12 = r12;
    ctx->rflags = rflags;
    ctx->r10 = r10;
    ctx->r9 = r9;
    ctx->r8 = r8;
    ctx->rbp = rbp;
    ctx->rdi = rdi;
    ctx->rsi = rsi;
    ctx->rsp = user_rsp;
    ctx->rip = user_rip;
    ctx->rbx = rbx;
    ctx->rax = rax;

    ctx->cs = 0x1b;
    ctx->ss = 0x23;

    ctx->can_sysret = true;
}

void context::load_from(const thread::context * ctx)
{
    r15 = ctx->r15;
    r14 = ctx->r14;
    r13 = ctx->r13;
    r12 = ctx->r12;
    rflags = ctx->can_sysret ? ctx->rflags : ctx->r11;
    r10 = ctx->r10;
    r9 = ctx->r9;
    r8 = ctx->r8;
    rbp = ctx->rbp;
    rdi = ctx->rdi;
    rsi = ctx->rsi;
    user_rsp = ctx->can_sysret ? ctx->rsp : ctx->rdx;
    user_rip = ctx->can_sysret ? ctx->rip : ctx->rcx;
    rbx = ctx->rbx;
    rax = ctx->rax;

    if (!ctx->can_sysret)
    {
        iret_rip = ctx->rip;
        iret_cs = ctx->cs;
        iret_rflags = ctx->rflags;
        iret_rsp = ctx->rsp;
        iret_ss = ctx->ss;
    }
}

extern "C" void syscall_handler_stub();

// Register interpretation at syscall:
//  - syscall number: rax
//  - return address: rcx (automatically set, the old value of rcx is discarded)
//  - userspace stack: rdx (semi-automatically set, the old value of rdx is discarded)
//  - arguments: rdi, rsi, r8, r9, r10, r12 up to r15

// Register status at return:
//  - return value: rax
//  - rcx: return address
//  - rest of registers: saved values
extern "C" void syscall_handler(context ctx)
{
    if (ctx.rax > syscall_count)
    {
        PANIC("TODO: handle a process invoking an invalid syscall");
    }

    // TODO: check that the syscall instruction is at the correct offset from the start of the vdso VMO

    auto previous_thread = cpu::get_core_local_storage()->current_thread;
    syscall_table[ctx.rax](ctx);
    auto new_thread = cpu::get_core_local_storage()->current_thread;

    if (new_thread != previous_thread)
    {
        ctx.save_to(previous_thread->get_context());
        ctx.load_from(new_thread->get_context());
    }
}

namespace
{
    constexpr auto ia32_efer = 0xc0000080;
    constexpr auto ia32_star = 0xc0000081;
    constexpr auto ia32_lstar = 0xc0000082;
    constexpr auto ia32_cstar = 0xc0000083;
    constexpr auto ia32_sfmask = 0xc0000084;
}

void initialize()
{
    auto stack = kernel::vm::allocate_address_range(32 * 4096);
    for (int i = 1; i < 32; ++i)
    {
        vm::map_physical(stack + i * 4096, stack + (i + 1) * 4096, pmm::pop(0));
    }

    cpu::get_core_local_storage()->kernel_syscall_stack = (stack + 32 * 4096).value();

    cpu::wrmsr(ia32_efer, cpu::rdmsr(ia32_efer) | 1);
    cpu::wrmsr(ia32_star, (0x8ull << 32) | (0x8ull << 48));
    cpu::wrmsr(ia32_lstar, reinterpret_cast<std::uint64_t>(&syscall_handler_stub));
}
}
