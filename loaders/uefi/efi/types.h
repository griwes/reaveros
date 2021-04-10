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

#include <cstddef>
#include <cstdint>

namespace efi_loader
{
using EFI_HANDLE = void *;
using EFI_STATUS = std::size_t;

struct EFI_GUID
{
    std::uint32_t data1;
    std::uint16_t data2;
    std::uint16_t data3;
    std::uint8_t data4[8];
};

struct EFI_TABLE_HEADER
{
    std::uint64_t signature;
    std::uint32_t revision;
    std::uint32_t header_size;
    std::uint32_t crc32;
    std::uint32_t reserved;
};

enum class EFI_ALLOCATE_TYPE
{
    allocate_any_pages,
    allocate_max_address,
    allocate_address,
    max_allocate_type
};

enum class EFI_MEMORY_TYPE : std::uint32_t
{
    efi_reserved_memory_type,
    efi_loader_code,
    efi_loader_data,
    efi_boot_services_code,
    efi_boot_services_data,
    efi_runtime_services_code,
    efi_runtime_services_data,
    efi_conventional_memory,
    efi_unusable_memory,
    efi_acpi_reclaim_memory,
    efi_acpi_memory_nvs,
    efi_memory_mapped_io,
    efi_memory_mapped_io_port_space,
    efi_pal_code,
    efi_persistent_memory,
    efi_max_memory_type,

    reaveros_kernel = 0x80000000,
    reaveros_initrd = 0x80000001,
    reaveros_paging = 0x80000002,
    reaveros_memory_map = 0x80000003,
    reaveros_backbuffer = 0x80001000,
};

struct EFI_TIME
{
    std::uint16_t year;  // 1900 – 9999
    std::uint8_t month;  // 1 – 12
    std::uint8_t day;    // 1 – 31
    std::uint8_t hour;   // 0 – 23
    std::uint8_t minute; // 0 – 59
    std::uint8_t second; // 0 – 59
    std::uint8_t pad1;
    std::uint32_t nanosecond; // 0 – 999,999,999
    std::int16_t time_zone;   // -1440 to 1440 or 2047
    std::uint8_t daylight;
    std::uint8_t pad2;
};

struct EFI_MEMORY_DESCRIPTOR
{
    EFI_MEMORY_TYPE type;
    std::uintptr_t physical_start;
    std::uintptr_t virtual_start;
    std::uint64_t number_of_pages;
    std::uint64_t attributes;
};
}
