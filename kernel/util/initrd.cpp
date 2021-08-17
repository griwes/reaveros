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

#include "initrd.h"

#include "log.h"
#include "pointer_types.h"

namespace kernel::initrd
{
namespace
{
    phys_addr_t bootinit_address;
    std::size_t bootinit_physical_size;
    std::size_t nested_initrd_physical_size;
}

void initialize(std::size_t memmap_size, boot_protocol::memory_map_entry * memmap)
{
    log::println("[BOOT] Finding and starting the boot init process...");
    auto entry = boot_protocol::find_entry(memmap_size, memmap, boot_protocol::memory_type::initrd);
    if (!entry)
    {
        PANIC("Initrd not found!");
    }

    log::println(" > Initrd address: {:#018x}.", entry->physical_start);
    log::println(" > Initrd physical size: {} bytes.", entry->length);

    auto magic_c = phys_ptr_t<const char>(phys_addr_t(entry->physical_start));
    auto magic = phys_ptr_t<std::uint64_t>(phys_addr_t(entry->physical_start));

    char magic_ref[] = "MDINITRD";
    if (std::string_view{ magic_ref } != std::string_view{ magic_c.value(), 8 })
    {
        PANIC("Initrd invalid, magic number does not match ({:#018x})!", *magic);
    }

    auto size_be = magic_c + 8;
    std::uint64_t size_native = 0;

    for (std::size_t i = 0; i < 64; i += 8)
    {
        size_native |= *size_be << (56 - i);
        size_be = size_be + 1;
    }

    if (size_native > entry->length)
    {
        PANIC("Initrd invalid, memmap size is {}, but initrd size is {:x}!", entry->length, size_native);
    }

    log::println(" > Initrd logical size: {} bytes.", size_native);

    std::uint8_t checksum = 0;
    auto base_u8 = phys_ptr_t<std::uint8_t>(phys_addr_t(entry->physical_start));

    for (auto it = base_u8.value(); it != base_u8.value() + size_native; ++it)
    {
        checksum += *it;
    }

    if (checksum != 0)
    {
        PANIC("Initrd invalid, checksum is {}, not zero!", checksum);
    }

    auto bootinit_size_be = size_be;
    std::uint64_t bootinit_size_native = 0;

    for (std::size_t i = 0; i < 64; i += 8)
    {
        bootinit_size_native |= *bootinit_size_be << (56 - i);
        bootinit_size_be = bootinit_size_be + 1;
    }

    bootinit_address = phys_addr_t(entry->physical_start + 0x1000);

    log::println(" > Bootinit logical size: {} bytes.", bootinit_size_native);

    if (bootinit_size_native > size_native - 0x1000)
    {
        PANIC("Bootinit claimed to be larger than the entire initrd!");
    }

    bootinit_physical_size = (bootinit_size_native + 4095) & ~4095ull;

    log::println(" > Bootinit physical size: {} bytes.", bootinit_physical_size);

    nested_initrd_physical_size = entry->length - 0x1000 - bootinit_physical_size;

    log::println(" > Nested initrd physical size: {} bytes.", nested_initrd_physical_size);
}
}
