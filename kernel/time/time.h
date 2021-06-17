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

#pragma once

#include <chrono>
#include <type_traits>

namespace kernel::time
{
void initialize();

class event_token
{
};

class timer
{
public:
    template<typename Rep, typename Period, typename Context>
    requires(std::is_trivially_copyable_v<Context> && sizeof(Context) <= 8) event_token
        one_shot(std::chrono::duration<Rep, Period> dur, void (*fptr)(Context), Context ctx)
    {
        std::uint64_t ctx_i;
        std::memcpy(&ctx_i, &ctx, sizeof(ctx));

        return do_one_shot(
            std::chrono::duration_cast<std::chrono::nanoseconds>(dur),
            +[](void * fptr, std::uint64_t ctx_i)
            {
                auto fptr_typed = reinterpret_cast<void (*)(Context)>(fptr);
                Context ctx;
                std::memcpy(&ctx, &ctx_i, sizeof(Context));
                fptr_typed(ctx);
            },
            reinterpret_cast<void *>(fptr),
            ctx_i);
    }

    virtual event_token do_one_shot(
        std::chrono::nanoseconds dur,
        void (*fptr)(void *, std::uint64_t),
        void * erased_fptr,
        std::uint64_t ctx) = 0;
};

timer & high_precision_clock();
}
