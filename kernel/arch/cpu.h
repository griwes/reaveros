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

#ifdef __amd64__
#include "amd64/cpu/core.h"
#include "amd64/cpu/cpu.h"

#define arch_namespace amd64

namespace kernel::arch::cpu
{
using amd64::cpu::core;
using amd64::cpu::get_core_count;
using amd64::cpu::initialize;
}
#else
#error "unknown architecture"
#endif

namespace kernel::arch::cpu
{
struct arch_independent_core
{
    core * native;

    auto id()
    {
        return native->id();
    }

    auto & scheduler()
    {
        return *native->scheduler();
    }
};

inline auto get_current_core()
{
    return arch_independent_core{ arch_namespace::cpu::get_current_core() };
}

inline auto get_core_by_id(std::size_t id)
{
    return arch_independent_core{ arch_namespace::cpu::get_core_by_id(id) };
}
}

#undef arch_namespace
