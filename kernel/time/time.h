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

#include "../util/chained_allocator.h"
#include "../util/tree_heap.h"

#include <chrono>
#include <type_traits>

namespace kernel::time
{
class timer;

void initialize();
void initialize_multicore();
void register_high_precision_timer(timer *);
timer & get_high_precision_timer(bool main = false);

class event_token
{
public:
    event_token(timer * tmr, std::size_t id) : _timer(tmr), _id(id)
    {
    }

    void cancel() const;

private:
    timer * _timer;
    std::size_t _id;
};

class timer
{
public:
    using duration = std::chrono::nanoseconds;

    template<typename Rep, typename Period>
    event_token one_shot(std::chrono::duration<Rep, Period> dur, void (*fptr)())
    {
        return _do(
            std::chrono::duration_cast<std::chrono::nanoseconds>(dur),
            +[](void * fptr, std::uint64_t)
            {
                auto fptr_typed = reinterpret_cast<void (*)()>(fptr);
                fptr_typed();
            },
            reinterpret_cast<void *>(fptr),
            0,
            _mode::one_shot);
    }

    template<typename Rep, typename Period, typename Context>
    requires(std::is_trivially_copyable_v<Context> && sizeof(Context) <= 8) event_token
        one_shot(std::chrono::duration<Rep, Period> dur, void (*fptr)(Context), Context ctx)
    {
        std::uint64_t ctx_i;
        std::memcpy(&ctx_i, &ctx, sizeof(ctx));

        return _do(
            std::chrono::duration_cast<std::chrono::nanoseconds>(dur),
            +[](void * fptr, std::uint64_t ctx_i)
            {
                auto fptr_typed = reinterpret_cast<void (*)(Context)>(fptr);
                Context ctx;
                std::memcpy(&ctx, &ctx_i, sizeof(Context));
                fptr_typed(ctx);
            },
            reinterpret_cast<void *>(fptr),
            ctx_i,
            _mode::one_shot);
    }

    void cancel(std::size_t id);
    std::chrono::time_point<timer> now();

    static void handle(timer * self);

protected:
    enum class _mode
    {
        one_shot,
        periodic
    };

    event_token _do(
        std::chrono::nanoseconds dur,
        void (*fptr)(void *, std::uint64_t),
        void * erased_fptr,
        std::uint64_t ctx,
        _mode mode);

    void _handle();
    void _schedule_next();

    virtual void _update_now() = 0;
    virtual void _one_shot_after(std::chrono::nanoseconds) = 0;

    std::size_t _usage = 0;
    std::chrono::nanoseconds _min_tick;
    std::chrono::nanoseconds _max_tick;
    std::chrono::time_point<timer> _now;

private:
    struct _timer_descriptor : util::treeable<_timer_descriptor>
    {
        _timer_descriptor() = default;

        using fptr = void (*)(void *, std::uint64_t);

        _timer_descriptor * parent = nullptr;

        std::size_t id = 0;
        std::chrono::time_point<timer> trigger_time;
        fptr callback;
        void * erased_callback;
        std::uint64_t context;
    };

    struct _timer_descriptor_comparator
    {
        bool operator()(const _timer_descriptor & lhs, const _timer_descriptor & rhs) const;
    };

    util::tree_heap<_timer_descriptor, _timer_descriptor_comparator> _heap;
    std::size_t _next_id = 0;
};
}
