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

#include "acpi.h"
#include "../../../util/log.h"
#include "../../../util/pointer_types.h"

#include <cstddef>
#include <cstdint>

namespace
{
struct [[gnu::packed]] description_header
{
    char signature[4];
    std::uint32_t length;
    std::uint8_t revision;
    std::uint8_t checksum;
    char oemid[6];
    std::uint64_t oem_table_id;
    std::uint32_t oem_revision;
    std::uint32_t creator_id;
    std::uint32_t creator_revision;

    bool validate(const char (&sign)[5])
    {
        if (std::memcmp(signature, sign, 4) != 0)
        {
            return false;
        }

        auto self = reinterpret_cast<std::uint8_t *>(this);
        std::uint8_t sum = 0;
        for (auto i = 0u; i < length; ++i)
        {
            sum += self[i];
        }

        return !sum;
    }
};

struct [[gnu::packed]] rsdt : public description_header
{
    kernel::phys_ptr_t<description_header, std::uint32_t> tables[1];
};

struct [[gnu::packed]] xsdt : public description_header
{
    kernel::phys_ptr_t<description_header> tables[1];
};

struct [[gnu::packed]] rsdp
{
    char signature[8];
    std::uint8_t checksum;
    char oemid[6];
    std::uint8_t revision;
    kernel::phys_ptr_t<rsdt, std::uint32_t> rsdt_pointer;
    std::uint32_t length;
    kernel::phys_ptr_t<xsdt> xsdt_pointer;
    std::uint8_t extended_checksum;
    std::uint8_t reserved[3];

    bool validate()
    {
        if (std::memcmp(signature, "RSD PTR ", 8) != 0)
        {
            return false;
        }

        auto self = reinterpret_cast<std::uint8_t *>(this);
        std::uint8_t sum = 0;
        for (auto i = 0; i < 20; ++i)
        {
            sum += self[i];
        }

        if (sum != 0)
        {
            return false;
        }

        if (revision == 0)
        {
            return true;
        }

        sum = 0;
        for (auto i = 0u; i < length; ++i)
        {
            sum += self[i];
        }

        return !sum;
    }
};

struct [[gnu::packed]] madt_entry_header
{
    std::uint8_t type;
    std::uint8_t length;
};

struct [[gnu::packed]] madt_lapic : public madt_entry_header
{
    std::uint8_t acpi_id;
    std::uint8_t apic_id;
    std::uint32_t flags;
};

struct [[gnu::packed]] madt_ioapic : public madt_entry_header
{
    std::uint8_t ioapic_id;
    std::uint8_t reserved;
    kernel::phys_addr32_t address;
    std::uint32_t interrupt_base;
};

struct [[gnu::packed]] madt_interrupt_source_override : public madt_entry_header
{
    std::uint8_t bus;
    std::uint8_t source;
    std::uint32_t interrupt;
    std::uint16_t flags;
};

struct [[gnu::packed]] madt_nmi : public madt_entry_header
{
    std::uint16_t flags;
    std::uint32_t interrupt;
};

struct [[gnu::packed]] madt_lapic_nmi : public madt_entry_header
{
    std::uint8_t acpi_id;
    std::uint16_t flags;
    std::uint8_t lapic_interrupt;
};

struct [[gnu::packed]] madt_lapic_base_override : public madt_entry_header
{
    std::uint16_t reserved;
    kernel::phys_addr_t lapic_base;
};

struct [[gnu::packed]] madt_x2apic : public madt_entry_header
{
    std::uint16_t reserved;
    std::uint32_t apic_id;
    std::uint32_t flags;
    std::uint32_t acpi_id;
};

struct [[gnu::packed]] madt_x2apic_nmi : public madt_entry_header
{
    std::uint16_t flags;
    std::uint32_t acpi_id;
    std::uint8_t lapic_interrupt;
    std::uint8_t reserved[3];
};

struct [[gnu::packed]] madt : public description_header
{
    static const constexpr char expected_signature[] = "APIC";

    kernel::phys_addr32_t lapic_base;
    std::uint32_t flags;
    madt_entry_header entries[1];
};

kernel::phys_ptr_t<rsdt> root_rsdt{ nullptr };
kernel::phys_ptr_t<xsdt> root_xsdt{ nullptr };

struct address_structure
{
    std::uint8_t address_space_id;
    std::uint8_t register_bit_width;
    std::uint8_t register_bit_offset;
    std::uint8_t reserved;
    kernel::phys_addr_t address;
} __attribute__((packed));

struct hpet : public description_header
{
    static const constexpr char expected_signature[] = "HPET";

    std::uint8_t hardware_rev_id;
    std::uint8_t comparator_count : 5;
    std::uint8_t counter_size : 1;
    std::uint8_t reserved : 1;
    std::uint8_t legacy_replacement : 1;
    kernel::pci_vendor_t pci_vendor_id;
    address_structure address;
    std::uint8_t hpet_number;
    std::uint16_t minimum_tick;
    std::uint8_t page_protection;
} __attribute__((packed));

template<typename Table, typename Root>
kernel::phys_ptr_t<Table> find_table(Root root)
{
    for (auto i = 0ull; i < (root->length - 36) / sizeof(root->tables); i++)
    {
        auto table_ptr = root->tables[i];
        if (table_ptr->validate(Table::expected_signature))
        {
            return kernel::phys_ptr_t<Table>{ static_cast<Table *>(table_ptr.value()) };
        }
    }

    return {};
}

template<typename Table>
kernel::phys_ptr_t<Table> find_table()
{
    if (root_xsdt.value())
    {
        return find_table<Table>(root_xsdt);
    }

    return find_table<Table>(root_rsdt);
}
}

namespace kernel::acpi
{
void initialize(std::size_t, phys_addr_t acpi_root)
{
    log::println("[ACPI] Initializing kernel ACPI structures...");

    auto rsdp_ptr = phys_ptr_t<rsdp>(acpi_root);
    if (!rsdp_ptr->validate())
    {
        PANIC("RSDP {} does not validate!", rsdp_ptr.value());
    }

    do
    {
        if (rsdp_ptr->revision >= 2)
        {
            if (rsdp_ptr->xsdt_pointer->validate("XSDT"))
            {
                root_xsdt = rsdp_ptr->xsdt_pointer;
                log::println(" > Using XSDT at {}.", root_xsdt.raw_value());
                break;
            }

            log::println(" > XSDT not valid, falling back to RSDT.");
        }

        if (rsdp_ptr->rsdt_pointer->validate("RSDT"))
        {
            root_rsdt = rsdp_ptr->rsdt_pointer;
            log::println(" > Using RSDT at {}.", root_rsdt.raw_value());
            break;
        }

        PANIC("Did not find valid ACPI root structures! RDSP: {}.", acpi_root.value());
    } while (false);
}

kernel::acpi::madt_result parse_madt(arch::cpu::core * cores_storage, std::size_t max_core_count)
{
    auto madt_ptr = find_table<madt>();
    if (!madt_ptr)
    {
        PANIC("Failed to find MADT!");
    }

    log::println(" > Found MADT at {}.", madt_ptr.raw_value());

    auto lapic_address = phys_addr_t{ madt_ptr->lapic_base };

    std::size_t detected_cores = 0;
    auto entry = madt_ptr->entries;

    entry = madt_ptr->entries;

    while (reinterpret_cast<std::uintptr_t>(entry) - reinterpret_cast<std::uintptr_t>(madt_ptr.value())
           < madt_ptr->length)
    {
        switch (entry->type)
        {
            case 0:
            {
                auto lapic = static_cast<madt_lapic *>(entry);
                if (lapic->flags & 1)
                {
                    log::println(
                        " > Found an active LAPIC entry, ID: {}, UID: {}.", lapic->apic_id, lapic->acpi_id);
                    cores_storage[detected_cores++].initialize_ids(lapic->apic_id, lapic->acpi_id);
                }

                break;
            }

            case 1:
            {
                auto ioapic = static_cast<madt_ioapic *>(entry);
                log::println(
                    " > Found an I/O APIC entry, ID: {}, handling vectors from: {}.",
                    ioapic->ioapic_id,
                    ioapic->interrupt_base);
                break;
            }

            case 2:
            {
                auto iso = static_cast<madt_interrupt_source_override *>(entry);
                log::println(" > Found a redirection entry: {} -> {}.", iso->source, iso->interrupt);
                break;
            }

            case 3:
            {
                auto nmi = static_cast<madt_nmi *>(entry);
                log::println(" > Found a global NMI entry: {}.", nmi->interrupt);
                break;
            }

            case 5:
            {
                auto lapic_override = static_cast<madt_lapic_base_override *>(entry);
                lapic_address = lapic_override->lapic_base;
                break;
            }

            case 9:
            {
                auto x2apic = static_cast<madt_x2apic *>(entry);
                if (x2apic->flags & 1)
                {
                    log::println(
                        " > Found an active x2APIC entry, ID: {}, UID: {}.",
                        x2apic->apic_id,
                        x2apic->acpi_id);
                    cores_storage[detected_cores++].initialize_ids(x2apic->apic_id, x2apic->acpi_id);
                }
                break;
            }
        }

        entry =
            reinterpret_cast<madt_entry_header *>(reinterpret_cast<std::uintptr_t>(entry) + entry->length);
    }

    entry = madt_ptr->entries;

    while (reinterpret_cast<std::uintptr_t>(entry) - reinterpret_cast<std::uintptr_t>(madt_ptr.value())
           < madt_ptr->length)
    {
        switch (entry->type)
        {
            case 4:
            {
                auto lapic_nmi = static_cast<madt_lapic_nmi *>(entry);

                if (lapic_nmi->acpi_id == 0xff)
                {
                    log::println(" > Found a LAPIC NMI entry for all cores: {}.", lapic_nmi->lapic_interrupt);
                    for (auto i = 0ull; i < detected_cores; ++i)
                    {
                        cores_storage[i].set_nmi(lapic_nmi->lapic_interrupt, lapic_nmi->flags);
                    }
                    break;
                }

                for (auto idx = 0ull; idx < detected_cores; ++idx)
                {
                    if (cores_storage[idx].acpi_id() == lapic_nmi->acpi_id)
                    {
                        log::println(
                            " > Found a LAPIC NMI entry for core {}: {}.",
                            cores_storage[idx].apic_id(),
                            lapic_nmi->lapic_interrupt);
                        cores_storage[idx].set_nmi(lapic_nmi->lapic_interrupt, lapic_nmi->flags);
                        break;
                    }

                    if (idx == detected_cores - 1)
                    {
                        log::println(
                            " > Warning: found a LAPIC NMI entry {} for an unknown ACPI ID {}.",
                            lapic_nmi->lapic_interrupt,
                            lapic_nmi->acpi_id);
                    }
                }

                break;
            }

            case 10:
            {
                auto x2apic_nmi = static_cast<madt_x2apic_nmi *>(entry);

                if (x2apic_nmi->acpi_id == 0xff)
                {
                    log::println(
                        " > Found a x2APIC NMI entry for all cores: {}.", x2apic_nmi->lapic_interrupt);
                    for (auto i = 0ull; i < detected_cores; ++i)
                    {
                        cores_storage[i].set_nmi(x2apic_nmi->lapic_interrupt, x2apic_nmi->flags);
                    }
                    break;
                }

                for (auto idx = 0ull; idx < detected_cores; ++idx)
                {
                    if (cores_storage[idx].acpi_id() == x2apic_nmi->acpi_id)
                    {
                        log::println(
                            " > Found a x2APIC NMI entry for core {}: {}.",
                            cores_storage[idx].apic_id(),
                            x2apic_nmi->lapic_interrupt);
                        cores_storage[idx].set_nmi(x2apic_nmi->lapic_interrupt, x2apic_nmi->flags);
                        break;
                    }

                    if (idx == detected_cores - 1)
                    {
                        log::println(
                            " > Warning: found a x2APIC NMI entry {} for an unknown ACPI ID {}.",
                            x2apic_nmi->lapic_interrupt,
                            x2apic_nmi->acpi_id);
                    }
                }

                break;
            }
        }

        entry =
            reinterpret_cast<madt_entry_header *>(reinterpret_cast<std::uintptr_t>(entry) + entry->length);
    }

    if (detected_cores > max_core_count)
    {
        log::println(
            " > Warning: detected more cores than supported, only the first {} will be used.",
            max_core_count);
        return { max_core_count, lapic_address };
    }

    return { detected_cores, lapic_address };
}

hpet_result parse_hpet()
{
    auto hpet_ptr = find_table<hpet>();

    log::println(" > Found HPET table.");
    log::println(
        " > Number: {}, PCI vendor ID: {:04x}.", hpet_ptr->hpet_number, hpet_ptr->pci_vendor_id.value());
    log::println(
        " > Number of comparators: {}, 64 bit counter? {}.",
        hpet_ptr->comparator_count + 1,
        static_cast<bool>(hpet_ptr->counter_size));
    log::println(" > Min tick: {}.", hpet_ptr->minimum_tick);

    if (hpet_ptr->address.address_space_id == 1)
    {
        PANIC("Found HPET, but in system I/O space, not in system memory!");
    }

    log::println(" > Base address: {:#018x}.", hpet_ptr->address.address.value());

    return { hpet_ptr->address.address, hpet_ptr->minimum_tick };
}
}
