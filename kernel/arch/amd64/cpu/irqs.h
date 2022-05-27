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

#pragma once

#include "thread.h"

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace kernel::amd64::irq
{
static const constexpr auto free_start = 0x20;
static const constexpr auto free_end = 0xe0;

static const constexpr auto parallel_exec_start = 0xe0;
static const constexpr auto parallel_exec_count = 0x10;

static const constexpr auto lapic_timer = 0xf0;
static const constexpr auto hpet_timer = 0xf1;
static const constexpr auto scheduling_trigger = 0xf2;
static const constexpr auto lapic_spurious = 0xff;

struct [[gnu::packed]] context
{
    std::uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    std::uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    std::uint64_t number, error;
    std::uint64_t rip, cs, rflags, rsp, ss;

    void save_to(thread::context *) const;
    void load_from(const thread::context *);
};

extern "C" void interrupt_handler(context ctx);

using erased_irq_handler = void (*)(context &, void *, std::uint64_t);
void register_erased_handler(std::uint8_t, erased_irq_handler, void *, std::uint64_t);

void register_handler(std::uint8_t irqn, void (*fptr)(context &));

template<typename Context>
requires(std::is_trivially_copyable_v<Context> && sizeof(Context) <= 8) void register_handler(
    std::uint8_t irqn,
    void (*fptr)(context &, Context),
    Context ctx)
{
    std::uint64_t ctx_i;
    std::memcpy(&ctx_i, &ctx, sizeof(ctx));

    register_erased_handler(
        irqn,
        +[](context & irq_context, void * fptr, std::uint64_t ctx_i)
        {
            auto fptr_typed = reinterpret_cast<void (*)(context &, Context)>(fptr);
            Context ctx;
            std::memcpy(&ctx, &ctx_i, sizeof(Context));
            fptr_typed(irq_context, ctx);
        },
        reinterpret_cast<void *>(fptr),
        ctx_i);
}
}
