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
#include "../../../memory/vm.h"
#include "../../../scheduler/scheduler.h"
#include "../../../scheduler/thread.h"
#include "../../../util/log.h"
#include "../../common/acpi/acpi.h"
#include "core.h"
#include "gdt.h"
#include "idt.h"
#include "lapic.h"

namespace kernel::amd64::cpu
{
namespace
{
    static const constexpr auto max_core_count = 1024;
    core cores[max_core_count];
    std::size_t core_count = max_core_count;
}

namespace detail_for_mp
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

    auto bsp_core = get_core_by_apic_id(lapic::id());
    bsp_core->initialize_gdt();
    bsp_core->load_gdt();

    idt::initialize();
    idt::load();

    wrmsr(0xc0000101, reinterpret_cast<std::uint64_t>(bsp_core->get_core_local_storage_ptr()));
    wrmsr(0xc0000102, reinterpret_cast<std::uint64_t>(bsp_core->get_core_local_storage_ptr()));
}

void idle()
{
    asm volatile("sti");

    while (true)
    {
        asm volatile("hlt");
    }
}

void ap_initialize()
{
    lapic::ap_initialize();

    auto core = get_core_by_apic_id(lapic::id());
    core->initialize_gdt();
    core->load_gdt();
    idt::load();

    wrmsr(0xc0000101, reinterpret_cast<std::uint64_t>(core->get_core_local_storage_ptr()));
    wrmsr(0xc0000102, reinterpret_cast<std::uint64_t>(core->get_core_local_storage_ptr()));

    lapic_timer::ap_initialize();

    idle();
}

void switch_to_clean_state()
{
    log::println("[CPU] Switching to clean state...");

    log::println(" > Switching to upper-half stack...");
    auto stack = kernel::vm::allocate_address_range(32 * 4096);

    log::println(" > Top of the stack: {:#018x}.", (stack + 32 * 4096).value());

    for (int i = 1; i < 32; ++i)
    {
        vm::map_physical(stack + i * 4096, stack + (i + 1) * 4096, pmm::pop(0));
    }

    std::uint64_t rbp;
    asm volatile("mov %%rbp, %0" : "=r"(rbp)::"memory");

    // first thing to pop from old stack is prior rbp
    // the second (+8 bytes) is the return address
    auto ret = *reinterpret_cast<std::uint64_t *>(rbp + 8);

    *reinterpret_cast<std::uint64_t *>(stack.value() + 32 * 4096 - 2 * 8) =
        (stack + 32 * 4096).value();                                         // fake pushed rbp
    *reinterpret_cast<std::uint64_t *>(stack.value() + 32 * 4096 - 8) = ret; // real return address

    asm volatile("mov %%rax, %%rbp; mov %%rax, %%rsp" ::"a"(stack.value() + 32 * 4096 - 2 * 8) : "memory");

    log::println(" > New stack installed.");

    vm::unmap_lower_half();

    asm volatile("mov %%rbp, %%rsp; pop %%rbp; ret" ::: "memory");
}

core * get_current_core()
{
    return get_core_local_storage()->current_core;
}

core * get_core_by_apic_id(std::uint32_t id)
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

core * get_core_by_id(std::size_t id)
{
    if (id < core_count)
    {
        return &cores[id];
    }

    PANIC("Requested a core for an unknown core ID {}!", id);
}

std::size_t get_core_count()
{
    return core_count;
}

bool interrupts_disabled()
{
    std::uintptr_t flags;
    asm volatile("pushfq; pop %%rax" : "=a"(flags));
    return !(flags & (1 << 9));
}
}
