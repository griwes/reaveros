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

#include "core.h"

#include <boot-arguments.h>

namespace kernel::amd64::cpu
{
inline void rdmsr(std::uint32_t msr, std::uint32_t & low, std::uint32_t & high)
{
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
}

inline std::uint64_t rdmsr(std::uint32_t msr)
{
    std::uint32_t low, high;
    rdmsr(msr, low, high);

    return (static_cast<std::uint64_t>(high) << 32) | low;
}

inline void wrmsr(std::uint32_t msr, std::uint32_t low, std::uint32_t high)
{
    asm volatile("wrmsr" ::"a"(low), "d"(high), "c"(msr));
}

inline void wrmsr(std::uint32_t msr, std::uint64_t val)
{
    wrmsr(msr, val & 0xFFFFFFFF, val >> 32);
}

namespace detail_for_mp
{
    core * get_core_array();
    std::size_t & get_core_count_ref();
}

void initialize();
[[noreturn]] void idle();
extern "C" void ap_initialize();
void switch_to_clean_state();
core * get_current_core();

core * get_core_by_apic_id(std::uint32_t id);
core * get_core_by_id(std::size_t id);
std::size_t get_core_count();
bool interrupts_disabled();
}
