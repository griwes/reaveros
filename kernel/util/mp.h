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

#include <cstdint>
#include <cstring>
#include <mutex>
#include <type_traits>

namespace kernel::mp
{
struct ipi_queue_item;

class ipi_queue
{
public:
    void push(ipi_queue_item *);
    void drain();

private:
    std::mutex _lock;
    ipi_queue_item * _head = nullptr;
    ipi_queue_item * _tail = nullptr;
};

enum class policy
{
    all,
    specific,
    // TODO:
    // others,
    // all_domain,
    // others_domain,
    // per_core,
    // per_chip,
    // per_domain
};

void initialize_parallel();
void erased_parallel_execute(
    policy,
    void (*)(void *, std::uintptr_t),
    void *,
    std::uint64_t,
    std::uintptr_t = 0);

void parallel_execute(policy, void (*fptr)(), std::uintptr_t target = 0);

inline void parallel_execute(void (*fptr)(), std::uintptr_t target = 0)
{
    parallel_execute(policy::all, fptr, target);
}

template<typename Context>
requires(std::is_trivially_copyable_v<Context> && sizeof(Context) <= 8) void parallel_execute(
    policy pol,
    void (*fptr)(Context),
    Context ctx,
    std::uintptr_t target = 0)
{
    std::uint64_t ctx_i;
    std::memcpy(&ctx_i, &ctx, sizeof(ctx));

    erased_parallel_execute(
        pol,
        +[](void * fptr, std::uint64_t ctx_i)
        {
            auto fptr_typed = reinterpret_cast<void (*)(Context)>(fptr);
            Context ctx;
            std::memcpy(&ctx, &ctx_i, sizeof(Context));
            fptr_typed(ctx);
        },
        reinterpret_cast<void *>(fptr),
        ctx_i,
        target);
}

template<typename Context>
requires(std::is_trivially_copyable_v<Context> && sizeof(Context) <= 8) void parallel_execute(
    void (*fptr)(Context),
    Context ctx,
    std::uintptr_t target = 0)
{
    return parallel_execute(policy::all, fptr, ctx, target);
}
}
