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

#include "instance.h"

#include "../arch/cpu.h"
#include "scheduler.h"
#include "thread.h"

namespace kernel::scheduler
{
instance::instance() = default;

instance::~instance() = default;

void instance::initialize(instance * parent)
{
    _parent = parent;

    if (parent)
    {
        _next_child = parent->_next_child;
        _parent->_children = this;
    }

    _idle_thread = util::make_intrusive<thread>(get_kernel_process());
    _current_thread = _idle_thread;
}

void instance::schedule(util::intrusive_ptr<thread> thread)
{
    if (_current_thread == thread)
    {
        return;
    }

    if (_current_thread != _idle_thread)
    {
        _current_thread->timestamp = time::get_high_precision_timer().now();
        _threads.push(std::move(_current_thread));
    }

    _threads.push(std::move(thread));
    _current_thread = _threads.pop();

    auto cls = arch::cpu::get_core_local_storage();
    auto old_thread = std::exchange(cls->current_thread, _current_thread);

    if (old_thread->get_container()->get_vas() != _current_thread->get_container()->get_vas())
    {
        arch::vm::set_asid(_current_thread->get_container()->get_vas()->get_asid());
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
