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

#include "lapic.h"
#include "../../../util/log.h"
#include "../../../util/pointer_types.h"
#include "cpu.h"
#include "irqs.h"

#include <cstdint>
#include <new>

namespace kernel::amd64::lapic
{
namespace
{
    class xapic_t
    {
    public:
        enum class _registers_rw
        {
            local_apic_id = 0x20,
            task_priority = 0x80,
            logical_destination = 0xd0,
            destination_format = 0xe0,
            spurious_interrupt_vector = 0xf0,
            lvt_cmci = 0x2f0,
            interrupt_command_low = 0x300,
            interrupt_command_high = 0x310,
            lvt_timer = 0x320,
            lvt_thermal_sensor = 0x330,
            lvt_pmc = 0x340,
            lvt_lint0 = 0x350,
            lvt_lint1 = 0x360,
            lvt_error = 0x370,
            timer_initial_count = 0x380,
            timer_divide_configuration = 0x3e0
        };

        enum class _registers_r
        {
            local_apic_version = 0x30,
            arbitration_priority_register = 0x90,
            processor_priority_register = 0xa0,
            remote_read_register = 0xc0,
            in_service_31_0 = 0x100,
            in_service_63_32 = 0x110,
            in_service_95_64 = 0x120,
            in_service_127_96 = 0x130,
            in_service_159_128 = 0x140,
            in_service_191_160 = 0x150,
            in_service_223_192 = 0x160,
            in_service_255_224 = 0x170,
            trigger_mode_31_0 = 0x180,
            trigger_mode_63_32 = 0x190,
            trigger_mode_95_64 = 0x1a0,
            trigger_mode_127_96 = 0x1b0,
            trigger_mode_159_128 = 0x1c0,
            trigger_mode_191_160 = 0x1d0,
            trigger_mode_223_192 = 0x1e0,
            trigger_mode_255_224 = 0x1f0,
            interrupt_request_31_0 = 0x200,
            interrupt_request_63_32 = 0x210,
            interrupt_request_95_64 = 0x220,
            interrupt_request_127_96 = 0x230,
            interrupt_request_159_128 = 0x240,
            interrupt_request_191_160 = 0x250,
            interrupt_request_223_192 = 0x260,
            interrupt_request_255_224 = 0x270,
            error_status = 0x280,
            timer_current_count = 0x390
        };

        enum class _registers_w
        {
            eoi = 0xb0
        };

        xapic_t(kernel::phys_addr_t base) : _register{ base }
        {
            _write(_registers_rw::destination_format, _read(_registers_rw::destination_format) & 0xF0000000);
            _write(_registers_rw::logical_destination, 0xFF000000);

            if (((_read(_registers_r::local_apic_version) >> 16) & 0xFF) == 6)
            {
                _write(_registers_rw::lvt_cmci, 0x10000);
            }

            _write(_registers_rw::lvt_error, 0x10000);
            _write(_registers_rw::lvt_lint0, 0x10000);
            _write(_registers_rw::lvt_lint1, 0x10000);
            _write(_registers_rw::lvt_pmc, 0x10000);
            _write(_registers_rw::lvt_thermal_sensor, 0x10000);
            _write(_registers_rw::lvt_timer, irq::lapic_timer);

            _write(_registers_rw::spurious_interrupt_vector, irq::lapic_spurious | 0x100);

            log::println(
                " > Initialized xAPIC. BSP ID: {}, APIC version: {}, number of LVTs: {}.",
                _read(_registers_rw::local_apic_id),
                _read(_registers_r::local_apic_version) & 0xFF,
                ((_read(_registers_r::local_apic_version) >> 16) & 0xFF) + 1);
        }

    private:
        phys_addr_t _register;

    public:
        std::uint32_t _read(_registers_r reg) const
        {
            return *phys_ptr_t<std::uint32_t>{ _register + static_cast<std::uintptr_t>(reg) };
        }

        std::uint32_t _read(_registers_rw reg) const
        {
            return *phys_ptr_t<std::uint32_t>{ _register + static_cast<std::uintptr_t>(reg) };
        }

        void _write(_registers_rw reg, std::uint32_t value) const
        {
            *phys_ptr_t<std::uint32_t>{ _register + static_cast<std::uintptr_t>(reg) } = value;
        }

        void _write(_registers_w reg, std::uint32_t value) const
        {
            *phys_ptr_t<std::uint32_t>{ _register + static_cast<std::uintptr_t>(reg) } = value;
        }
    };

    class x2apic_t
    {
    public:
        enum class _registers_rw
        {
            apic_base = 0x01b,
            task_priority = 0x808,
            spurious_interrupt_vector = 0x80f,
            error_status = 0x828,
            lvt_cmci = 0x82f,
            interrupt_command = 0x830,
            lvt_timer = 0x832,
            lvt_thermal_sensor = 0x833,
            lvt_pmc = 0x834,
            lvt_lint0 = 0x835,
            lvt_lint1 = 0x836,
            lvt_error = 0x837,
            timer_initial_count = 0x838,
            timer_divide_configuration = 0x83e
        };

        enum class _registers_r
        {
            local_apic_id = 0x802,
            local_apic_version = 0x803,
            processor_priority_register = 0x80a,
            logical_destination = 0x80d,
            in_service_31_0 = 0x810,
            in_service_63_32 = 0x811,
            in_service_95_64 = 0x812,
            in_service_127_96 = 0x813,
            in_service_159_128 = 0x814,
            in_service_191_160 = 0x815,
            in_service_223_192 = 0x816,
            in_service_255_224 = 0x817,
            trigger_mode_31_0 = 0x818,
            trigger_mode_63_32 = 0x819,
            trigger_mode_95_64 = 0x81a,
            trigger_mode_127_96 = 0x81b,
            trigger_mode_159_128 = 0x81c,
            trigger_mode_191_160 = 0x81d,
            trigger_mode_223_192 = 0x81e,
            trigger_mode_255_224 = 0x81f,
            interrupt_request_31_0 = 0x820,
            interrupt_request_63_32 = 0x821,
            interrupt_request_95_64 = 0x822,
            interrupt_request_127_96 = 0x823,
            interrupt_request_159_128 = 0x824,
            interrupt_request_191_160 = 0x825,
            interrupt_request_223_192 = 0x826,
            interrupt_request_255_224 = 0x827,
            timer_current_count = 0x839
        };

        enum class _registers_w
        {
            eoi = 0x80b,
            self_ipi = 0x83f
        };

        x2apic_t()
        {
            _write(_registers_rw::apic_base, _read(_registers_rw::apic_base) | (1 << 10));

            if (((_read(_registers_r::local_apic_version) >> 16) & 0xFF) == 6)
            {
                _write(_registers_rw::lvt_cmci, 0x10000);
            }

            _write(_registers_rw::lvt_error, 0x10000);
            _write(_registers_rw::lvt_lint0, 0x10000);
            _write(_registers_rw::lvt_lint1, 0x10000);
            _write(_registers_rw::lvt_pmc, 0x10000);
            _write(_registers_rw::lvt_thermal_sensor, 0x10000);
            _write(_registers_rw::lvt_timer, irq::lapic_timer);

            _write(_registers_rw::spurious_interrupt_vector, irq::lapic_spurious | 0x100);

            log::println(
                " > Initialized x2APIC. BSP ID: {}, APIC version: {}, number of LVTs: {}.",
                _read(_registers_r::local_apic_id),
                _read(_registers_r::local_apic_version) & 0xFF,
                ((_read(_registers_r::local_apic_version) >> 16) & 0xFF) + 1);
        }

        std::uint64_t _read(_registers_r reg) const
        {
            return cpu::rdmsr(static_cast<std::uint32_t>(reg));
        }

        std::uint64_t _read(_registers_rw reg) const
        {
            return cpu::rdmsr(static_cast<std::uint32_t>(reg));
        }

        void _write(_registers_rw reg, std::uint64_t value) const
        {
            cpu::wrmsr(static_cast<std::uint32_t>(reg), value);
        }

        void _write(_registers_w reg, std::uint64_t value) const
        {
            cpu::wrmsr(static_cast<std::uint32_t>(reg), value);
        }
    };

    union lapic_storage_t
    {
        lapic_storage_t()
        {
        }

        ~lapic_storage_t()
        {
        }

        xapic_t xapic;
        x2apic_t x2apic;
    } lapic_storage;

    bool x2apic_enabled = false;
}

#define cpuid(id, a, b, c, d) asm volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(id))

void initialize(phys_addr_t lapic_base)
{
    std::uint32_t _, ecx;
    cpuid(1, _, _, ecx, _);

    if (ecx & (1ull << 21))
    {
        log::println(" > Enabling x2APIC mode.");
        new (&lapic_storage.x2apic) x2apic_t();
        x2apic_enabled = true;
    }

    else
    {
        log::println(" > Enabling xAPIC mode.");
        new (&lapic_storage.xapic) xapic_t(lapic_base);
    }
}

std::uint32_t id()
{
    return x2apic_enabled ? lapic_storage.x2apic._read(x2apic_t::_registers_r::local_apic_id)
                          : lapic_storage.xapic._read(xapic_t::_registers_rw::local_apic_id);
}

void eoi(std::uint8_t number)
{
    if (number != irq::lapic_spurious)
    {
        x2apic_enabled ? lapic_storage.x2apic._write(x2apic_t::_registers_w::eoi, 0)
                       : lapic_storage.xapic._write(xapic_t::_registers_w::eoi, 0);
    }
}

void write_timer_divisor(std::uint8_t val)
{
    std::uint32_t bit_pattern;
    switch (val)
    {
        case 1:
            bit_pattern = 0b1011;
            break;
        case 2:
            bit_pattern = 0b0000;
            break;
        case 4:
            bit_pattern = 0b0001;
            break;
        case 8:
            bit_pattern = 0b0010;
            break;
        case 16:
            bit_pattern = 0b0011;
            break;
        case 32:
            bit_pattern = 0b1000;
            break;
        case 64:
            bit_pattern = 0b1001;
            break;
        case 128:
            bit_pattern = 0b1010;
            break;

        default:
            PANIC("Invalid LAPIC timer divisor requested: {}!", val);
    }

    x2apic_enabled
        ? lapic_storage.x2apic._write(x2apic_t::_registers_rw::timer_divide_configuration, bit_pattern)
        : lapic_storage.xapic._write(xapic_t::_registers_rw::timer_divide_configuration, bit_pattern);
}

void write_timer_counter(std::uint32_t val)
{
    x2apic_enabled ? lapic_storage.x2apic._write(x2apic_t::_registers_rw::timer_initial_count, val)
                   : lapic_storage.xapic._write(xapic_t::_registers_rw::timer_initial_count, val);
}

std::uint32_t read_timer_counter()
{
    return x2apic_enabled ? lapic_storage.x2apic._read(x2apic_t::_registers_r::timer_current_count)
                          : lapic_storage.xapic._read(xapic_t::_registers_r::timer_current_count);
}

void ipi(std::uint32_t target_apic_id, ipi_type type, std::uint8_t data)
{
    std::uint32_t cmd_low = 0;

    switch (type)
    {
        case ipi_type::init:
            cmd_low = 5 << 8;
            break;

        case ipi_type::sipi:
            cmd_low = (6 << 8) | data;
            break;

        case ipi_type::generic:
        case ipi_type::nmi:
            PANIC("Unsupported IPI type selected.");
    }

    if (x2apic_enabled)
    {
        lapic_storage.x2apic._write(
            x2apic_t::_registers_rw::interrupt_command,
            (static_cast<std::uint64_t>(target_apic_id) << 32) | cmd_low);
    }
    else
    {
        lapic_storage.xapic._write(xapic_t::_registers_rw::interrupt_command_high, target_apic_id << 24);
        lapic_storage.xapic._write(xapic_t::_registers_rw::interrupt_command_low, cmd_low);
    }
}
}
