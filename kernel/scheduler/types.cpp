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

#include "types.h"

#include "../arch/cpu.h"
#include "../arch/ipi.h"
#include "../arch/irqs.h"
#include "../util/interrupt_control.h"
#include "scheduler.h"
#include "thread.h"

namespace kernel::scheduler
{
aggregate::aggregate() = default;

aggregate::~aggregate() = default;

std::size_t aggregate::average_load()
{
    auto _ = std::lock_guard(_lock);

    std::size_t child_count = 0;
    std::size_t total_load = 0;

    for (auto child = _children; child; child = child->_next_sibling)
    {
        ++child_count;
        total_load += child->average_load();
    }

    return total_load / child_count;
}

void aggregate::schedule(util::intrusive_ptr<thread> thread)
{
    auto _ = std::lock_guard(_lock);

    interface * lowest = nullptr;
    std::size_t lowest_al = -1;

    for (auto child = _children; child; child = child->_next_sibling)
    {
        if (auto al = child->average_load(); al < lowest_al)
        {
            lowest = child;
            lowest_al = al;
        }
    }

    if (!lowest)
    {
        PANIC("didn't find any candidate children schedulers");
    }

    lowest->schedule(std::move(thread));
}

void aggregate::add_child(interface * child)
{
    auto _ = std::lock_guard(_lock);

    if (!_children)
    {
        _children = child;
        return;
    }

    for (auto current = _children;; current = current->_next_sibling)
    {
        if (!current->_next_sibling)
        {
            current->_next_sibling = child;
            return;
        }
    }
}

instance::instance() = default;

instance::~instance() = default;

void instance::initialize(aggregate * parent, void * core)
{
    _parent = parent;
    _core = core;

    if (parent)
    {
        parent->add_child(this);
    }

    _idle_thread = util::make_intrusive<thread>(get_kernel_process());
    _current_thread = _idle_thread;
}

std::size_t instance::average_load()
{
    auto _ = std::lock_guard(_lock);

    return _threads.size() * 100;
}

void instance::schedule(util::intrusive_ptr<thread> thread)
{
    if (!arch::cpu::interrupts_disabled())
    {
        PANIC("scheduler::schedule called with interrupts enabled (it's meant to only be called "
              "in interrupt or syscall handlers)!");
    }

    auto lock = std::lock_guard(_lock);

    if (_current_thread == thread)
    {
        PANIC("rescheduling the current thread!");
    }

    _threads.push(std::move(thread));
    _setup_preemption(lock);
}

util::intrusive_ptr<thread> instance::deschedule()
{
    auto lock = std::lock_guard(_lock);

    auto ret = std::move(_current_thread);
    ret->timestamp = time::get_high_precision_timer().now();
    _reschedule(lock);

    return ret;
}

void instance::scheduling_trigger()
{
    auto lock = std::lock_guard(_lock);
    _setup_preemption(lock);
}

void instance::_reschedule(std::lock_guard<std::mutex> & lock)
{
    if (arch::cpu::get_core_local_storage()->current_core->get_scheduler() != this) [[unlikely]]
    {
        PANIC("_reschedule called on a scheduler instance not of the current core!");
    }

    if (_current_thread && _current_thread != _idle_thread)
    {
        _current_thread->timestamp = time::get_high_precision_timer().now();
        _threads.push(std::move(_current_thread));
    }

    if (_threads.size())
    {
        _current_thread = _threads.pop();
    }
    else
    {
        _current_thread = _idle_thread;
    }

    auto cls = arch::cpu::get_core_local_storage();
    auto old_thread = std::exchange(cls->current_thread, _current_thread);

    if (old_thread->get_container()->get_vas() != _current_thread->get_container()->get_vas())
    {
        arch::vm::set_asid(_current_thread->get_container()->get_vas()->get_asid());
    }

    _setup_preemption(lock);
}

void instance::_setup_preemption(std::lock_guard<std::mutex> &)
{
    if (arch::cpu::get_core_local_storage()->current_core->get_scheduler() != this)
    {
        auto core = reinterpret_cast<arch::cpu::core *>(_core);
        arch::ipi::ipi(core->id(), arch::irq::scheduling_trigger);
    }

    if (_threads.size())
    {
        if (_preemption_token)
        {
            _preemption_token->cancel();
            _preemption_token.reset();
        }

        _preemption_token = time::get_preemption_timer().one_shot(
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)) / 100,
            +[](instance * self)
            {
                auto lock = std::lock_guard(self->_lock);
                self->_reschedule(lock);
            },
            this);
    }
}

util::intrusive_ptr<thread> instance::get_idle_thread()
{
    return _idle_thread;
}

util::intrusive_ptr<thread> instance::get_current_thread()
{
    return _current_thread;
}

bool instance::_thread_timestamp_compare::operator()(const thread & lhs, const thread & rhs) const
{
    return lhs.timestamp < rhs.timestamp;
}
}
