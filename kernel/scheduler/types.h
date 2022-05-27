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

#include "../time/time.h"
#include "../util/intrusive_ptr.h"
#include "../util/tree_heap.h"

#include <optional>

namespace kernel::scheduler
{
class thread;
class aggregate;

class interface
{
public:
    virtual ~interface() = default;

    virtual std::size_t average_load() = 0;
    virtual void schedule(util::intrusive_ptr<thread> thread) = 0;

    friend class aggregate;

protected:
    std::mutex _lock;

    aggregate * _parent = nullptr;
    interface * _next_sibling = nullptr;
};

class aggregate : public interface
{
public:
    aggregate();
    virtual ~aggregate() override;

    virtual std::size_t average_load() override;
    virtual void schedule(util::intrusive_ptr<thread> thread) override;

    void add_child(interface * child);

private:
    interface * _children = nullptr;
};

class instance : public interface
{
public:
    instance();
    virtual ~instance() override;

    void initialize(aggregate * parent, void * core_ptr);

    virtual std::size_t average_load() override;
    virtual void schedule(util::intrusive_ptr<thread> thread) override;
    util::intrusive_ptr<thread> deschedule();
    void scheduling_trigger();

    util::intrusive_ptr<thread> get_idle_thread();
    util::intrusive_ptr<thread> get_current_thread();

private:
    void _reschedule(std::lock_guard<std::mutex> & lock);
    void _setup_preemption(std::lock_guard<std::mutex> & lock);

    struct _thread_timestamp_compare
    {
        bool operator()(const thread & lhs, const thread & rhs) const;
    };

    void * _core; // void * to break circular header dependency
    std::optional<time::timer::event_token> _preemption_token;

    util::intrusive_ptr<thread> _idle_thread;
    util::intrusive_ptr<thread> _current_thread;

    util::tree_heap<thread, _thread_timestamp_compare, util::intrusive_ptr_preserve_count_traits> _threads;
};
}
