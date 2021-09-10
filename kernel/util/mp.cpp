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

#include "mp.h"

#include "../arch/cpu.h"
#include "../arch/ipi.h"
#include "../arch/irqs.h"
#include "log.h"

#include <atomic>
#include <mutex>

namespace kernel::mp
{
namespace
{
    struct parallel_slot
    {
        std::mutex lock;
        bool wait = true;
        void (*fptr)(void *, std::uint64_t);
        void * erased_fptr;
        std::uint64_t context;
        std::atomic<std::size_t> unfinished_cores;
        std::size_t irq_slot;
    };

    parallel_slot slots[arch::irq::parallel_exec_count]{};

    std::atomic<std::size_t> next_slot{};
}

void initialize_parallel()
{
    for (std::size_t i = 0; i < arch::irq::parallel_exec_count; ++i)
    {
        slots[i].irq_slot = arch::irq::parallel_exec_start + i;
        arch::irq::register_handler(
            slots[i].irq_slot,
            +[](arch::irq::context &, std::size_t id)
            {
                auto & slot = slots[id];

                auto fptr = slot.fptr;
                auto erased_fptr = slot.erased_fptr;
                auto context = slot.context;
                auto wait = slot.wait;

                if (!wait)
                {
                    --slot.unfinished_cores;
                }

                fptr(erased_fptr, context);

                if (wait)
                {
                    --slot.unfinished_cores;
                }
            },
            i);
    }
}

void erased_parallel_execute(
    policy pol,
    void (*fptr)(void *, std::uintptr_t),
    void * erased_fptr,
    std::uint64_t context,
    std::uintptr_t target)
{
    auto & slot = slots[next_slot++ % arch::irq::parallel_exec_count];

    std::lock_guard lock(slot.lock);

    slot.fptr = fptr;
    slot.erased_fptr = erased_fptr;
    slot.context = context;

    switch (pol)
    {
        case policy::all_no_wait:
            slot.wait = false;
            [[fallthrough]];

        case policy::all:
            slot.unfinished_cores = arch::cpu::get_core_count();
            arch::ipi::broadcast(arch::ipi::broadcast_target::others, slot.irq_slot);
            fptr(erased_fptr, context);
            --slot.unfinished_cores;
            break;

        case policy::others_no_wait:
            slot.wait = false;
            [[fallthrough]];

        case policy::others:
            slot.unfinished_cores = arch::cpu::get_core_count() - 1;
            arch::ipi::broadcast(arch::ipi::broadcast_target::others, slot.irq_slot);
            break;

        case policy::specific_no_wait:
            slot.wait = false;
            [[fallthrough]];

        case policy::specific:
            slot.unfinished_cores = 1;
            arch::ipi::ipi(target, slot.irq_slot);
            break;
    }

    while (slot.unfinished_cores.load(std::memory_order_relaxed) != 0)
    {
        // TODO: abstract
        asm volatile("pause");
    }

    slot.fptr = nullptr;
    slot.erased_fptr = nullptr;
    slot.context = 0;
    slot.wait = true;
}

void parallel_execute(policy pol, void (*fptr)(), std::uintptr_t target)
{
    erased_parallel_execute(
        pol,
        +[](void * fptr, std::uint64_t)
        {
            auto fptr_typed = reinterpret_cast<void (*)()>(fptr);
            fptr_typed();
        },
        reinterpret_cast<void *>(fptr),
        0,
        target);
}
}
