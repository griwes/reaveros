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

#include "../util/log.h"

namespace kernel::arch::ipi
{
enum class broadcast_target
{
    self = 1,
    all,
    others
};
}

#ifdef __amd64__
#include "amd64/cpu/cpu.h"
#include "amd64/cpu/lapic.h"

namespace kernel::arch::ipi
{
inline auto to_arch_target(broadcast_target target)
{
    switch (target)
    {
        case broadcast_target::self:
            return amd64::lapic::broadcast_target::self;
        case broadcast_target::all:
            return amd64::lapic::broadcast_target::all;
        case broadcast_target::others:
            return amd64::lapic::broadcast_target::others;
        default:
            PANIC("Unknown broadcast target!");
    }
}

inline void ipi(std::size_t target_cpu_id, std::uint8_t irq)
{
    amd64::lapic::ipi(
        amd64::cpu::get_core_by_id(target_cpu_id)->apic_id(), amd64::lapic::ipi_type::generic, irq);
}

inline void ipi_nmi(std::size_t target_cpu_id);

inline void broadcast(broadcast_target target, std::uint8_t irq)
{
    amd64::lapic::broadcast(to_arch_target(target), amd64::lapic::ipi_type::generic, irq);
}

inline void broadcast_nmi(broadcast_target target);
}
#else
#error "unknown architecture"
#endif
