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

#include <climits>
#include "../arch/timers.h"
#include "../util/log.h"

#include "time.h"

namespace
{
kernel::time::timer * hpc = nullptr;
bool per_core_hpc_available = false;
}

namespace kernel::time
{
void initialize()
{
    log::println("[TIME] Initializing the time subsystem...");

    arch::timers::initialize();
}

void register_high_precision_timer(timer * high_precision)
{
    if (hpc)
    {
        PANIC("Tried to register a high precision timer with another already registered!");
    }

    hpc = high_precision;
}

timer & get_high_precision_timer()
{
    if (per_core_hpc_available) [[likely]]
    {
        PANIC("implement returning a per core high precision timer");
    }

    if (!hpc) [[unlikely]]
    {
        PANIC("High precision clock requested, but not registered!");
    }

    return *hpc;
}

void timer::handle(timer * self)
{
    self->_schedule_next();
}

event_token timer::_do(
    std::chrono::nanoseconds dur,
    void (*fptr)(void *, std::uint64_t),
    void * erased_fptr,
    std::uint64_t ctx,
    _mode mode)
{
    // TODO: lock

    _update_now();

    auto desc = std::make_unique<_timer_descriptor>();
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

    _heap.push(std::move(desc));

    _schedule_next();

    return event_token(nullptr, 0);
}

void timer::_schedule_next()
{
    // TODO: lock

    _update_now();

    for (auto top = _heap.peek(); top->trigger_time < _now; top = _heap.peek())
    {
        auto desc = _heap.pop();
        // drop lock

        desc->callback(desc->erased_callback, desc->context);

        _update_now();

        if (desc->id & (static_cast<std::size_t>(1) << (sizeof(std::size_t) * CHAR_BIT - 1)))
        {
            PANIC("implement periodic timer events already");
        }
    }

    // TODO: lock

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
}
