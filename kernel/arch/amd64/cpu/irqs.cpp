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

#include "irqs.h"

#include "../../../scheduler/thread.h"
#include "../../../util/log.h"
#include "core.h"
#include "cpu.h"
#include "lapic.h"

#include <mutex>

namespace kernel::amd64::irq
{
void context::save_to(thread::context * thctx) const
{
    thctx->rax = rax;
    thctx->rbx = rbx;
    thctx->rcx = rcx;
    thctx->rdx = rdx;
    thctx->rsi = rsi;
    thctx->rdi = rdi;
    thctx->rbp = rbp;
    thctx->rsp = rsp;
    thctx->r8 = r8;
    thctx->r9 = r9;
    thctx->r10 = r10;
    thctx->r11 = r11;
    thctx->r12 = r12;
    thctx->r13 = r13;
    thctx->r14 = r14;
    thctx->r15 = r15;

    thctx->rip = rip;
    thctx->rflags = rflags;
    thctx->cs = cs;
    thctx->ss = ss;
}

void context::load_from(const thread::context * thctx)
{
    rax = thctx->rax;
    rbx = thctx->rbx;
    rcx = thctx->rcx;
    rdx = thctx->rdx;
    rsi = thctx->rsi;
    rdi = thctx->rdi;
    rbp = thctx->rbp;
    rsp = thctx->rsp;
    r8 = thctx->r8;
    r9 = thctx->r9;
    r10 = thctx->r10;
    r11 = thctx->r11;
    r12 = thctx->r12;
    r13 = thctx->r13;
    r14 = thctx->r14;
    r15 = thctx->r15;

    rip = thctx->rip;
    rflags = thctx->rflags;
    cs = thctx->cs;
    ss = thctx->ss;
}

namespace
{
    struct irq_handler
    {
        std::mutex lock;
        bool valid = false;
        erased_irq_handler fptr;
        void * erased_fptr;
        std::uint64_t context;
    };

    irq_handler irq_handlers[256];
}

void handle(context & ctx)
{
    auto & handler = irq_handlers[ctx.number];

    std::lock_guard lg(handler.lock);

    if (!handler.valid)
    {
        PANIC("Unexpected IRQ: {:x}, {}", ctx.number, ctx.error);
    }

    auto previous_thread = cpu::get_core_local_storage()->current_thread;
    handler.fptr(ctx, handler.erased_fptr, handler.context);
    auto new_thread = cpu::get_core_local_storage()->current_thread;

    if (new_thread != previous_thread)
    {
        ctx.save_to(previous_thread->get_context());
        ctx.load_from(new_thread->get_context());

        if (new_thread->get_container()->get_vas() != previous_thread->get_container()->get_vas())
        {
            vm::set_asid(new_thread->get_container()->get_vas()->get_asid());
        }
    }

    if (ctx.number >= 32)
    {
        lapic::eoi(ctx.number);
    }
}

void register_erased_handler(
    std::uint8_t irqn,
    erased_irq_handler fptr,
    void * erased_fptr,
    std::uint64_t ctx)
{
    auto & handler = irq_handlers[irqn];

    std::lock_guard lg(handler.lock);

    if (handler.valid)
    {
        PANIC("Overriding an already registered IRQ: {:x}!", irqn);
    }

    handler.fptr = fptr;
    handler.erased_fptr = erased_fptr;
    handler.context = ctx;
    handler.valid = true;
}

void register_handler(std::uint8_t irqn, void (*fptr)(context &))
{
    register_erased_handler(
        irqn,
        +[](context & ctx, void * fptr, std::uint64_t)
        {
            auto fptr_typed = reinterpret_cast<void (*)(context &)>(fptr);
            fptr_typed(ctx);
        },
        reinterpret_cast<void *>(fptr),
        0);
}
}
