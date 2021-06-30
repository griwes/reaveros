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
#include "../../../util/log.h"
#include "../../common/acpi/acpi.h"
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
namespace detail_for_smp
{
    core * get_core_array()
    {
        return cores;
    }

    std::size_t & get_core_count_ref()
    {
        return core_count;
    }
}

void initialize()
{
    log::println("[CPU] Initializing CPU...");

    auto madt_result = acpi::parse_madt(cores, core_count);
    core_count = madt_result.core_count;

    lapic::initialize(madt_result.lapic_base);

    auto bsp_core = get_current_core();
    bsp_core->initialize_gdt();
    bsp_core->load_gdt();
    idt::initialize();
    idt::load();
}

void ap_initialize()
{
    asm volatile("cli; hlt");
}

core * get_current_core()
{
    auto id = lapic::id();
    return get_core_by_id(id);
}

core * get_core_by_id(std::uint32_t id)
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

std::size_t get_core_count()
{
    return core_count;
}

phys_addr_t get_asid()
{
    std::uint64_t cr3;
    asm volatile("mov %%cr3, %%rax" : "=a"(cr3));
    return phys_addr_t(cr3);
}
}
