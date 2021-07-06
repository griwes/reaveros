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

#include "mp.h"

#include "../../../memory/vm.h"
#include "../../../time/time.h"
#include "../cpu/cpu.h"
#include "../cpu/lapic.h"
#include "../memory/vm.h"

#include <cstddef>
#include <cstdint>

extern "C" std::uint8_t trampoline_start[];
extern "C" std::uint8_t trampoline_end[];
extern "C" std::uint8_t trampoline_asid_slot[];
extern "C" std::uint8_t trampoline_stack_slot[];
extern "C" std::uint8_t trampoline_flag_slot[];

namespace kernel::amd64::mp
{
void boot()
{
    log::println("[CPU] Booting APs...");

    auto trampoline_size = trampoline_end - trampoline_start;
    trampoline_size += 4095;
    trampoline_size &= ~4095ull;

    auto asid_slot_offset = trampoline_asid_slot - trampoline_start;
    auto stack_slot_offset = trampoline_stack_slot - trampoline_start;
    auto flag_slot_offset = trampoline_flag_slot - trampoline_start;

    auto & core_count = cpu::detail_for_mp::get_core_count_ref();
    auto cores = cpu::detail_for_mp::get_core_array();

    // INIT IPI
    for (std::size_t i = 0; i < core_count; ++i)
    {
        auto apic_id = cores[i].apic_id();

        if (apic_id == cpu::get_current_core()->apic_id())
        {
            continue;
        }

        log::println(" > Sending INIT IPI to core #{} (APIC ID: {})...", i, apic_id);
        lapic::ipi(apic_id, lapic::ipi_type::init, 0);
    }

    volatile bool timer_triggered = false;

    auto handler = +[](volatile bool * flag) { *flag = true; };

    using namespace std::literals;
    time::get_high_precision_timer().one_shot(10ms, handler, &timer_triggered);

    asm volatile("sti");

    while (!timer_triggered)
    {
        asm volatile("hlt");
    }

    timer_triggered = false;

    auto base_raw = pmm::get_sub_1M_bottom();
    base_raw = base_raw ? base_raw : 0x1000;
    auto top = pmm::get_sub_1M_top();
    auto base = phys_ptr_t<std::uint8_t>(phys_addr_t(base_raw));

    log::println(" > Using memory range from {:#010x} to {:#010x}.", base_raw, top);

    vm::map_physical(virt_addr_t(base_raw), virt_addr_t(top), phys_addr_t(base_raw));

    auto boot_at_once = (top - base_raw) / trampoline_size;

    for (std::size_t booted = 0; booted < core_count; booted += boot_at_once)
    {
        for (std::size_t i = booted; i < booted + boot_at_once && i < core_count; ++i)
        {
            if (cores[i].apic_id() == cpu::get_current_core()->apic_id())
            {
                continue;
            }

            std::memcpy((base + trampoline_size * (i - booted)).value(), trampoline_start, trampoline_size);

            *phys_ptr_t<std::uint64_t>(base + trampoline_size * (i - booted) + asid_slot_offset) =
                cpu::get_asid().value();

            cores[i]._boot_flag = const_cast<volatile std::uint8_t *>(
                (base + trampoline_size * (i - booted) + flag_slot_offset).value());

            auto stack = kernel::vm::allocate_address_range(8192);
            vm::map_physical(stack + 4096, stack + 8192, pmm::pop_4k());

            *phys_ptr_t<std::uint64_t>(base + trampoline_size * (i - booted) + stack_slot_offset) =
                (stack + 8192).value();
        }

        // SIPI
        for (std::size_t i = booted; i < booted + boot_at_once && i < core_count; ++i)
        {
            auto apic_id = cores[i].apic_id();

            if (apic_id == cpu::get_current_core()->apic_id())
            {
                continue;
            }

            log::println(" > Sending Startup IPI to core #{} (APIC ID: {})...", i, apic_id);
            lapic::ipi(apic_id, lapic::ipi_type::sipi, (0x1000 + trampoline_size * (i - booted)) >> 12);
        }

        time::get_high_precision_timer().one_shot(500us, handler, &timer_triggered);

        while (!timer_triggered)
        {
            asm volatile("hlt");
        }

        timer_triggered = false;

        // 2nd SIPI, if not started
        for (std::size_t i = booted; i < booted + boot_at_once && i < core_count; ++i)
        {
            auto apic_id = cores[i].apic_id();

            if (apic_id == cpu::get_current_core()->apic_id())
            {
                continue;
            }

            if (!*(cores[i]._boot_flag))
            {
                log::println(" > Sending secondary Startup IPI to core #{} (APIC ID: {})...", i, apic_id);
                lapic::ipi(apic_id, lapic::ipi_type::sipi, (0x1000 + trampoline_size * (i - booted)) >> 12);
            }
        }

        time::get_high_precision_timer().one_shot(500us, handler, &timer_triggered);

        while (!timer_triggered)
        {
            asm volatile("hlt");
        }

        timer_triggered = false;

        for (std::size_t i = booted; i < booted + boot_at_once && i < core_count; ++i)
        {
            if (cores[i].apic_id() == cpu::get_current_core()->apic_id())
            {
                continue;
            }

            if (*(cores[i]._boot_flag))
            {
                log::println(" > CPU #{} (APIC ID: {}) started up...", i, cores[i].apic_id());

                while (*cores[i]._boot_flag != 2)
                {
                    asm volatile("pause");
                }

                log::println(" > CPU #{} (APIC ID: {}) booted successfully.", i, cores[i].apic_id());

                cores[i]._id = i;
            }

            else
            {
                log::println(" > CPU#{} failed to boot!", cores[i].apic_id());

                for (std::size_t j = i; j < core_count - 1; ++j)
                {
                    log::println(
                        " > Moving CPU #{} (APIC ID: {}) into place of CPU #{} (APIC ID: {}).",
                        j + 1,
                        cores[j + 1].apic_id(),
                        j,
                        cores[j].apic_id());
                    cores[j] = cores[j + 1];
                }

                --i;
                --core_count;
                --booted;
            }
        }
    }

    for (std::size_t i = 0; i < core_count; ++i)
    {
        if (cores[i].apic_id() == cpu::get_current_core()->apic_id())
        {
            cores[i]._id = i;
            break;
        }
    }
}
}
