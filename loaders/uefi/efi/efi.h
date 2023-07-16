/*
 * Copyright © 2021, 2023 Michał 'Griwes' Dominiak
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

#include <boot-memmap.h>

#include <climits>

#ifndef EFIAPI
#ifdef _MSC_EXTENSIONS
#define EFIAPI __cdecl
#else
#define EFIAPI __attribute__((ms_abi))
#endif
#endif

#include "types.h"

namespace efi_loader
{
constexpr auto EFI_SYSTEM_TABLE_SIGNATURE = 0x5453595320494249;
constexpr auto EFI_2_60_SYSTEM_TABLE_REVISION = ((2 << 16) | (60));
constexpr auto EFI_2_50_SYSTEM_TABLE_REVISION = ((2 << 16) | (50));
constexpr auto EFI_2_40_SYSTEM_TABLE_REVISION = ((2 << 16) | (40));
constexpr auto EFI_2_31_SYSTEM_TABLE_REVISION = ((2 << 16) | (31));
constexpr auto EFI_2_30_SYSTEM_TABLE_REVISION = ((2 << 16) | (30));
constexpr auto EFI_2_20_SYSTEM_TABLE_REVISION = ((2 << 16) | (20));
constexpr auto EFI_2_10_SYSTEM_TABLE_REVISION = ((2 << 16) | (10));
constexpr auto EFI_2_00_SYSTEM_TABLE_REVISION = ((2 << 16) | (00));
constexpr auto EFI_1_10_SYSTEM_TABLE_REVISION = ((1 << 16) | (10));
constexpr auto EFI_1_02_SYSTEM_TABLE_REVISION = ((1 << 16) | (02));

constexpr auto EFI_SYSTEM_TABLE_REVISION = EFI_2_60_SYSTEM_TABLE_REVISION;
constexpr auto EFI_SPECIFICATION_VERSION = EFI_SYSTEM_TABLE_REVISION;

constexpr auto high_bit = 1ull << (sizeof(EFI_STATUS) * CHAR_BIT - 1);

constexpr auto EFI_SUCCESS = 0;
constexpr auto EFI_LOAD_ERROR = 1 | high_bit;
constexpr auto EFI_INVALID_PARAMETER = 2 | high_bit;
constexpr auto EFI_UNSUPPORTED = 3 | high_bit;
constexpr auto EFI_BAD_BUFFER_SIZE = 4 | high_bit;
constexpr auto EFI_BUFFER_TOO_SMALL = 5 | high_bit;
constexpr auto EFI_NOT_READY = 6 | high_bit;
constexpr auto EFI_DEVICE_ERROR = 7 | high_bit;
constexpr auto EFI_WRITE_PROTECTED = 8 | high_bit;
constexpr auto EFI_OUT_OF_RESOURCES = 9 | high_bit;
constexpr auto EFI_VOLUME_CORRUPTED = 10 | high_bit;
constexpr auto EFI_VOLUME_FULL = 11 | high_bit;
constexpr auto EFI_NO_MEDIA = 12 | high_bit;
constexpr auto EFI_MEDIA_CHANGED = 13 | high_bit;
constexpr auto EFI_NOT_FOUND = 14 | high_bit;
constexpr auto EFI_ACCESS_DENIED = 15 | high_bit;
constexpr auto EFI_NO_RESPONSE = 16 | high_bit;
constexpr auto EFI_NO_MAPPING = 17 | high_bit;
constexpr auto EFI_TIMEOUT = 18 | high_bit;
constexpr auto EFI_NOT_STARTED = 19 | high_bit;
constexpr auto EFI_ALREADY_STARTED = 20 | high_bit;
constexpr auto EFI_ABORTED = 21 | high_bit;
constexpr auto EFI_ICMP_ERROR = 22 | high_bit;
constexpr auto EFI_TFTP_ERROR = 23 | high_bit;
constexpr auto EFI_PROTOCOL_ERROR = 24 | high_bit;
constexpr auto EFI_INCOMPATIBLE_VERSION = 25 | high_bit;
constexpr auto EFI_SECURITY_VIOLATION = 26 | high_bit;
constexpr auto EFI_CRC_ERROR = 27 | high_bit;
constexpr auto EFI_END_OF_MEDIA = 28 | high_bit;
constexpr auto EFI_END_OF_FILE = 31 | high_bit;
constexpr auto EFI_INVALID_LANGUAGE = 32 | high_bit;
constexpr auto EFI_COMPROMISED_DATA = 33 | high_bit;
constexpr auto EFI_IP_ADDRESS_CONFLICT = 34 | high_bit;
constexpr auto EFI_HTTP_ERROR = 35 | high_bit;

struct EFI_SYSTEM_TABLE;

extern EFI_SYSTEM_TABLE * system_table;
extern EFI_HANDLE image_handle;

inline void initialize(EFI_SYSTEM_TABLE * table, EFI_HANDLE handle)
{
    system_table = table;
    image_handle = handle;
}

void * open_protocol_by_guid(EFI_HANDLE, const EFI_GUID &, const char *);
void * open_protocol_by_guid(const EFI_GUID &, const char *);

void * allocate_pages(std::size_t, EFI_MEMORY_TYPE type);
void deallocate_pages(void *, std::size_t);

struct acpi_information
{
    std::size_t revision;
    std::uintptr_t root;
};

acpi_information find_acpi_root();

struct image_info
{
    void * image_base;
    std::size_t image_size;
};

image_info get_image_info();

struct memory_map
{
    std::size_t key;
    std::size_t size;
    std::uint32_t efi_entry_size;
    char * efi_entries;
    boot_protocol::memory_map_entry * entries;

    EFI_MEMORY_DESCRIPTOR * get_efi_entry(std::size_t index)
    {
        return reinterpret_cast<EFI_MEMORY_DESCRIPTOR *>(efi_entries + efi_entry_size * index);
    }

    void account_for_stack(std::uintptr_t begin, std::uintptr_t end);
};

memory_map get_memory_map();
void exit_boot_services(const memory_map &);
}

#define PROTO_GUID(PROTO) EFI_##PROTO##_PROTOCOL_GUID
#define PROTOCOL(PROTO) EFI_##PROTO##_PROTOCOL
#define FIND_PROTOCOL_FROM_HANDLE(HANDLE, PROTO)                                                             \
    (reinterpret_cast<PROTOCOL(PROTO) *>(                                                                    \
        ::efi_loader::open_protocol_by_guid(HANDLE, PROTO_GUID(PROTO), #PROTO)))
#define FIND_PROTOCOL(PROTO)                                                                                 \
    (reinterpret_cast<PROTOCOL(PROTO) *>(::efi_loader::open_protocol_by_guid(PROTO_GUID(PROTO), #PROTO)))
