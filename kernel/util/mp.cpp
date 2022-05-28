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

#include "mp.h"

#include "../arch/cpu.h"
#include "../arch/ipi.h"
#include "../arch/irqs.h"
#include "../util/interrupt_control.h"
#include "log.h"

#include <atomic>
#include <mutex>

namespace kernel::mp
{
namespace
{
    ipi_queue_item * outgoing_ipi_items = nullptr;
}

struct ipi_queue_state
{
    void (*fptr)(void *, std::uintptr_t);
    void * erased_ptr;
    std::uintptr_t context;
    std::atomic<std::size_t> unfinished_cores{ 0 };
};

struct ipi_queue_item
{
    ipi_queue_item * next;
    ipi_queue_state * state;
};

void ipi_queue::push(ipi_queue_item * item)
{
    auto _ = std::lock_guard(_lock);

    if (!_head)
    {
        _tail = _head = item;
        return;
    }

    _tail = _tail->next = item;
}

void ipi_queue::drain()
{
    while (true)
    {
        ipi_queue_item * item;

        {
            auto _ = std::lock_guard(_lock);
            item = _head;
            if (_head)
            {
                _head = _head->next;
            }
            else
            {
                return;
            }
        }

        item->state->fptr(item->state->erased_ptr, item->state->context);
        --item->state->unfinished_cores;
    }
}

void initialize_parallel()
{
    arch::irq::register_handler(
        arch::irq::ipi_trigger,
        +[](arch::irq::context &)
        { arch::cpu::get_core_local_storage()->current_core->get_ipi_queue()->drain(); });

    auto core_count = arch::cpu::get_core_count();
    auto size = core_count * core_count * sizeof(ipi_queue_item);
    auto base = vm::allocate_address_range(size);
    for (std::size_t offset = 0; offset < size; offset += arch::vm::page_sizes[0])
    {
        arch::vm::map_physical(base + offset, base + offset + arch::vm::page_sizes[0], pmm::pop(0));
    }

    outgoing_ipi_items =
        new (reinterpret_cast<void *>(base.value())) ipi_queue_item[core_count * core_count]();
}

namespace
{
    bool matches(policy pol, std::uintptr_t target, std::uintptr_t core)
    {
        switch (pol)
        {
            case policy::all:
                return true;

            case policy::specific:
                return target == core;
        }
    }
}

void erased_parallel_execute(
    policy pol,
    void (*fptr)(void *, std::uintptr_t),
    void * erased_fptr,
    std::uint64_t context,
    std::uintptr_t target)
{
    ipi_queue_state state = { fptr, erased_fptr, context };
    auto core_count = arch::cpu::get_core_count();
    auto self_id = arch::cpu::get_core_local_storage()->current_core->id();

    if (pol == policy::specific)
    {
        state.unfinished_cores = 1;
        auto & item = outgoing_ipi_items[self_id * core_count + target];
        item.state = &state;
        arch::cpu::get_core_by_id(target)->get_ipi_queue()->push(&item);
    }

    else
    {
        for (std::size_t i = 0; i < arch::cpu::get_core_count(); ++i)
        {
            if (matches(pol, target, i))
            {
                ++state.unfinished_cores;
                auto & item = outgoing_ipi_items[self_id * core_count + i];
                item.state = &state;
                arch::cpu::get_core_by_id(i)->get_ipi_queue()->push(&item);
            }
        }
    }

    switch (pol)
    {
        case policy::all:
            arch::ipi::broadcast(arch::ipi::broadcast_target::others, arch::irq::ipi_trigger);
            break;

        case policy::specific:
            arch::ipi::ipi(target, arch::irq::ipi_trigger);
            break;
    }

    while (state.unfinished_cores != 0)
    {
        arch::cpu::get_core_local_storage()->current_core->get_ipi_queue()->drain();
    }
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
