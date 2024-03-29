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

#include "../arch/cpu.h"
#include "../arch/thread.h"
#include "../time/time.h"
#include "../util/intrusive_ptr.h"
#include "process.h"

namespace kernel::scheduler
{
class thread : public util::intrusive_ptrable<thread>
{
public:
    thread(util::intrusive_ptr<process> container);

    process * get_container() const
    {
        return _container.get();
    }

    arch::thread::context * get_context()
    {
        return &_context;
    }

    const arch::thread::context * get_context() const
    {
        return &_context;
    }

    arch::cpu::core * get_core() const
    {
        return _core;
    }

    thread * tree_parent = nullptr;
    std::chrono::time_point<time::timer> timestamp;

    using continuation_t = bool (*)(std::uintptr_t &, void *);
    using destructor_t = void (*)(void *);

    template<typename State>
    void set_continuation(continuation_t c, State state)
    {
        static_assert(sizeof(State) < 512, "too large continuation state");

        auto _ = std::lock_guard(_continuation_lock);

        if (_continuation)
        {
            PANIC("tried to set a continuation for a thread with an active continuation");
        }

        _continuation = c;
        new (&_continuation_state) State(std::move(state));
        _destructor = +[](void * ptr) { reinterpret_cast<State *>(ptr)->~State(); };
    }

    bool invoke_continuation(std::uintptr_t & return_register)
    {
        auto _ = std::lock_guard(_continuation_lock);

        if (_continuation)
        {
            auto ret = _continuation(return_register, &_continuation_state);
            if (ret)
            {
                _destructor(&_continuation_state);
            }
            _continuation = nullptr;
            return ret;
        }

        return true;
    }

private:
    util::intrusive_ptr<process> _container;

    arch::thread::context _context;
    arch::cpu::core * _core;

    std::mutex _continuation_lock;
    continuation_t _continuation = nullptr;
    destructor_t _destructor = nullptr;
    char _continuation_state[512];
};
}
