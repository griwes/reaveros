/*
 * Copyright © 2022 Michał 'Griwes' Dominiak
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

namespace kernel::amd64::cpu
{
inline bool interrupts_disabled()
{
    std::uint64_t flags = 0;
    asm volatile("pushfq; popq %0" : "=r"(flags)::"memory");
    return !(flags & (1 << 9));
}

inline bool disable_interrupts()
{
    auto ret = interrupts_disabled();
    if (!ret)
    {
        asm volatile("cli" ::: "memory");
    }
    return ret;
}

inline void enable_interrupts()
{
    asm volatile("sti" ::: "memory");
}
}
