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

#include "cpu.h"
#include "../../util/log.h"
#include "../common/acpi/acpi.h"
#include "core.h"
#include "gdt.h"
#include "idt.h"
#include "lapic.h"

namespace
{
static const constexpr auto max_core_count = 1024;
kernel::amd64::cpu::core cores[max_core_count];
std::size_t core_count = max_core_count;
}

namespace kernel::amd64::cpu
{
void initialize()
{
    log::println("[CPU] Initializing CPU...");

    auto madt_result = acpi::parse_madt(cores, core_count);
    core_count = madt_result.core_count;

    lapic::initialize(madt_result.lapic_base);

    auto bsp_core = current_core();
    bsp_core->initialize_gdt();
    bsp_core->load_gdt();
    idt::initialize();
    idt::load();

    // time::hpet::initialize();
    // time::real::initialize();
    // time::lapic::initialize();

    // smp::boot();
}

core * current_core()
{
    auto id = lapic::id();
    return core_by_id(id);
}

core * core_by_id(std::uint32_t id)
{
    for (auto i = 0ull; i < core_count; ++i)
    {
        if (cores[i].apic_id() == id)
        {
            return &cores[i];
        }
    }

    PANIC("Requested a core for an unknown APIC ID {}!", id);
}
}
