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

#include <climits>
#include "../arch/cpu.h"
#include "../arch/timers.h"
#include "../util/log.h"

#include "time.h"

namespace kernel::time
{
namespace
{
    timer * hpt = nullptr;
    bool per_core_hpt_available = false;
}

void initialize()
{
    log::println("[TIME] Initializing the time subsystem...");

    arch::timers::initialize();
}

void initialize_multicore()
{
    log::println("[TIME] Rebalancing timers across available cores...");

    arch::timers::multicore_initialize();
    per_core_hpt_available = true;
}

void register_high_precision_timer(timer * high_precision)
{
    if (hpt)
    {
        PANIC("Tried to register a high precision timer with another already registered!");
    }

    hpt = high_precision;
}

timer & get_high_precision_timer(bool main)
{
    if (!main && per_core_hpt_available) [[likely]]
    {
        return *arch::timers::get_high_precision_timer_for(arch::cpu::get_current_core()->id());
    }

    if (!hpt) [[unlikely]]
    {
        PANIC("Global high precision clock requested, but not registered!");
    }

    return *hpt;
}

timer & get_preemption_timer()
{
    return *arch::timers::get_preemption_timer();
}

std::chrono::time_point<timer> timer::now()
{
    util::interrupt_guard guard;
    std::unique_lock _(_lock);
    _update_now(_);
    return _now;
}

void timer::handle(timer * self)
{
    self->_schedule_next();
}

timer::event_token timer::_do(
    std::chrono::nanoseconds dur,
    void (*fptr)(void *, std::uint64_t),
    void * erased_fptr,
    std::uint64_t ctx,
    _mode mode)
{
    auto desc = util::make_intrusive<_timer_descriptor>();

    {
        util::interrupt_guard guard;
        std::unique_lock _(_lock);
        _update_now(_);

        desc->id = ++_next_id
            | (static_cast<std::size_t>(mode == _mode::periodic) << (sizeof(std::size_t) * CHAR_BIT - 1));
        desc->trigger_time = _now + dur;
        desc->callback = fptr;
        desc->erased_callback = erased_fptr;
        desc->context = ctx;

        if (_next_id & (static_cast<std::size_t>(1) << (sizeof(std::size_t) * CHAR_BIT - 1))) [[unlikely]]
        {
            PANIC("Somehow, timer IDs have been exhausted!");
        }

        _heap.push(desc);
    }

    _schedule_next();

    return event_token(nullptr, 0, std::move(desc));
}

void timer::_schedule_next()
{
    util::interrupt_guard guard;
    std::unique_lock lock(_lock);

    _update_now(lock);

    for (auto top = _heap.peek(); top && top->trigger_time <= _now; top = _heap.peek())
    {
        auto desc = _heap.pop();
        lock.unlock();

        if (desc->valid.load(std::memory_order_relaxed))
        {
            desc->callback(desc->erased_callback, desc->context);
        }

        lock.lock();
        _update_now(lock);

        if (desc->id & (static_cast<std::size_t>(1) << (sizeof(std::size_t) * CHAR_BIT - 1)))
        {
            PANIC("implement periodic timer events already");
        }
    }

    auto top = _heap.peek();
    if (!top)
    {
        return;
    }

    _one_shot_after(top->trigger_time - _now);
}

bool timer::_timer_descriptor_comparator::operator()(
    const timer::_timer_descriptor & lhs,
    const timer::_timer_descriptor & rhs) const
{
    return lhs.trigger_time < rhs.trigger_time;
}

void timer::event_token::cancel()
{
    _desc->valid.store(false, std::memory_order_relaxed);
    _desc.release(util::drop_count);
}
}
