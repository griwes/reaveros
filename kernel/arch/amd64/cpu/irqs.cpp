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

#include "irqs.h"

#include "../../../util/log.h"
#include "../cpu/lapic.h"

namespace
{
struct irq_handler
{
    bool valid = false;
    kernel::amd64::irq::erased_irq_handler fptr;
    void * erased_fptr;
    std::uint64_t context;
};

irq_handler irq_handlers[256];
}

namespace kernel::amd64::irq
{
void handle(context & ctx)
{
    // TODO: lock?

    auto & handler = irq_handlers[ctx.number];

    if (!handler.valid)
    {
        PANIC("Unexpected IRQ: {:x}, {}", ctx.number, ctx.error);
    }

    handler.fptr(ctx, handler.erased_fptr, handler.context);

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
    // TODO: lock?

    auto & handler = irq_handlers[irqn];

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
