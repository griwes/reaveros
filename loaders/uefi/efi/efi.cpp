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

#include "efi.h"
#include "../cpu/halt.h"
#include "console.h"
#include "system_table.h"
#include "types.h"

namespace efi_loader
{
constexpr auto EFI_BOOT_SERVICES_SIGNATURE = 0x56524553544f4f42;
constexpr auto EFI_BOOT_SERVICES_REVISION = EFI_SPECIFICATION_VERSION;

using EFI_RAISE_TPL = void (*)();
using EFI_RESTORE_TPL = void (*)();

using EFI_ALLOCATE_PAGES = EFIAPI EFI_STATUS (*)(EFI_ALLOCATE_TYPE, EFI_MEMORY_TYPE, std::size_t, void **);

using EFI_FREE_PAGES = void (*)();

using EFI_GET_MEMORY_MAP = EFIAPI EFI_STATUS (*)(
    std::size_t *,
    EFI_MEMORY_DESCRIPTOR *,
    std::size_t *,
    std::size_t *,
    std::uint32_t *);

using EFI_ALLOCATE_POOL = EFIAPI EFI_STATUS (*)(EFI_MEMORY_TYPE, std::size_t, void **);

using EFI_FREE_POOL = EFIAPI EFI_STATUS (*)(void *);

using EFI_CREATE_EVENT = void (*)();
using EFI_SET_TIMER = void (*)();
using EFI_WAIT_FOR_EVENT = void (*)();
using EFI_SIGNAL_EVENT = void (*)();
using EFI_CLOSE_EVENT = void (*)();
using EFI_CHECK_EVENT = void (*)();
using EFI_INSTALL_PROTOCOL_INTERFACE = void (*)();
using EFI_REINSTALL_PROTOCOL_INTERFACE = void (*)();
using EFI_UNINSTALL_PROTOCOL_INTERFACE = void (*)();
using EFI_HANDLE_PROTOCOL = void (*)();
using EFI_REGISTER_PROTOCOL_NOTIFY = void (*)();
using EFI_LOCATE_HANDLE = void (*)();
using EFI_LOCATE_DEVICE_PATH = void (*)();
using EFI_INSTALL_CONFIGURATION_TABLE = void (*)();
using EFI_IMAGE_LOAD = void (*)();
using EFI_IMAGE_START = void (*)();
using EFI_EXIT = void (*)();
using EFI_IMAGE_UNLOAD = void (*)();

using EFI_EXIT_BOOT_SERVICES = EFIAPI EFI_STATUS (*)(EFI_HANDLE, std::size_t);

using EFI_GET_NEXT_MONOTONIC_COUNT = void (*)();
using EFI_STALL = void (*)();
using EFI_SET_WATCHDOG_TIMER = void (*)();
using EFI_CONNECT_CONTROLLER = void (*)();
using EFI_DISCONNECT_CONTROLLER = void (*)();

constexpr auto EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL = 0x00000001;
constexpr auto EFI_OPEN_PROTOCOL_GET_PROTOCOL = 0x00000002;
constexpr auto EFI_OPEN_PROTOCOL_TEST_PROTOCOL = 0x00000004;
constexpr auto EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER = 0x00000008;
constexpr auto EFI_OPEN_PROTOCOL_BY_DRIVER = 0x00000010;
constexpr auto EFI_OPEN_PROTOCOL_EXCLUSIVE = 0x00000020;

using EFI_OPEN_PROTOCOL = EFIAPI EFI_STATUS (*)(
    EFI_HANDLE handle,
    EFI_GUID * protocol,
    void ** interface,
    EFI_HANDLE agent_handle,
    EFI_HANDLE controller_handle,
    std::uint32_t attributes);

using EFI_CLOSE_PROTOCOL = void (*)();
using EFI_OPEN_PROTOCOL_INFORMATION = void (*)();
using EFI_PROTOCOLS_PER_HANDLE = void (*)();
using EFI_LOCATE_HANDLE_BUFFER = void (*)();

using EFI_LOCATE_PROTOCOL =
    EFIAPI EFI_STATUS (*)(EFI_GUID * protocol, void * registration, void ** interface);

using EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES = void (*)();
using EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES = void (*)();
using EFI_CALCULATE_CRC32 = void (*)();
using EFI_COPY_MEM = void (*)();
using EFI_SET_MEM = void (*)();
using EFI_CREATE_EVENT_EX = void (*)();

struct EFI_BOOT_SERVICES
{
    EFI_TABLE_HEADER header;

    EFI_RAISE_TPL raise_tpl;
    EFI_RESTORE_TPL restore_tpl;

    EFI_ALLOCATE_PAGES allocate_pages;
    EFI_FREE_PAGES free_pages;
    EFI_GET_MEMORY_MAP get_memory_map;
    EFI_ALLOCATE_POOL allocate_pool;
    EFI_FREE_POOL free_pool;

    EFI_CREATE_EVENT create_event;
    EFI_SET_TIMER set_timer;
    EFI_WAIT_FOR_EVENT wait_forEvent;
    EFI_SIGNAL_EVENT signal_event;
    EFI_CLOSE_EVENT close_event;
    EFI_CHECK_EVENT check_event;

    EFI_INSTALL_PROTOCOL_INTERFACE install_protocol_interface;
    EFI_REINSTALL_PROTOCOL_INTERFACE reinstall_protocol_interface;
    EFI_UNINSTALL_PROTOCOL_INTERFACE uninstall_protocol_interface;
    EFI_HANDLE_PROTOCOL handle_protocol;
    void * reserved;
    EFI_REGISTER_PROTOCOL_NOTIFY register_protocol_notify;
    EFI_LOCATE_HANDLE locate_handle;
    EFI_LOCATE_DEVICE_PATH locate_devicePath;
    EFI_INSTALL_CONFIGURATION_TABLE install_configuration_table;

    EFI_IMAGE_LOAD load_image;
    EFI_IMAGE_START start_image;
    EFI_EXIT exit;
    EFI_IMAGE_UNLOAD unload_image;
    EFI_EXIT_BOOT_SERVICES exit_boot_services;

    EFI_GET_NEXT_MONOTONIC_COUNT get_next_monotonic_count;
    EFI_STALL stall;
    EFI_SET_WATCHDOG_TIMER set_watchdog_timer;

    EFI_CONNECT_CONTROLLER connect_controller;
    EFI_DISCONNECT_CONTROLLER disconnect_controller;

    EFI_OPEN_PROTOCOL open_protocol;
    EFI_CLOSE_PROTOCOL close_protocol;
    EFI_OPEN_PROTOCOL_INFORMATION open_protocol_information;

    EFI_PROTOCOLS_PER_HANDLE protocols_per_handle;
    EFI_LOCATE_HANDLE_BUFFER locate_handle_buffer;
    EFI_LOCATE_PROTOCOL locate_protocol;
    EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES install_multiple_protocol_interfaces;
    EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES uninstall_multiple_protocol_interfaces;
    EFI_CALCULATE_CRC32 calculate_crc32;

    EFI_COPY_MEM copy_mem;
    EFI_SET_MEM set_mem;
    EFI_CREATE_EVENT_EX create_event_ex;
};

EFI_SYSTEM_TABLE * system_table = nullptr;
EFI_HANDLE image_handle = {};

void * open_protocol_by_guid(const EFI_GUID & guid, const char * name)
{
    void * ret;
    auto status = system_table->boot_services->locate_protocol(const_cast<EFI_GUID *>(&guid), nullptr, &ret);

    if (status != EFI_SUCCESS)
    {
        console::print(u" > Failed to open protocol: ", name, u".\n\r");
        return nullptr;
    }

    return ret;
}

void * open_protocol_by_guid(EFI_HANDLE handle, const EFI_GUID & guid, const char * name)
{
    void * ret;
    auto status = system_table->boot_services->open_protocol(
        handle,
        const_cast<EFI_GUID *>(&guid),
        &ret,
        image_handle,
        nullptr,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    if (status != EFI_SUCCESS)
    {
        console::print(u" > Failed to open protocol: ", name, u".\n\r");
        return nullptr;
    }

    return ret;
}

void * allocate_pages(std::size_t size, EFI_MEMORY_TYPE type)
{
    auto num_pages = size ? (size + 4095) / 4096 : 1;

    void * ret = nullptr;
    switch (auto status = system_table->boot_services->allocate_pages(
                EFI_ALLOCATE_TYPE::allocate_any_pages, type, num_pages, &ret))
    {
        case EFI_SUCCESS:
            return ret;

        default:
            console::print(u"[ERR] Error allocating pages: ", status & ~high_bit, u".\n\r");
            halt();
    }
}

memory_map get_memory_map()
{
    std::size_t size = 0;
    std::size_t descriptor_size = 0;
    std::uint32_t descriptor_version = 0;

    switch (auto status = system_table->boot_services->get_memory_map(
                &size, nullptr, nullptr, &descriptor_size, &descriptor_version))
    {
        case EFI_BUFFER_TOO_SMALL:
            break;

        default:
            console::print(u"[ERR] Error getting memory map size: ", status & ~high_bit, u".\n\r");
            halt();
    }

    size /= descriptor_size;

    console::print(
        u" > Detected memory map descriptor version ",
        descriptor_version,
        u", size: ",
        descriptor_size,
        u".\n\r");
    console::print(u" > Memory map size prior to map buffer allocation: ", size, u".\n\r");

    // Amortize for the allocations we are about to make.
    size += 5;

    memory_map map;

    map.efi_entry_size = descriptor_size;
    map.efi_entries =
        reinterpret_cast<char *>(allocate_pages(descriptor_size * size, EFI_MEMORY_TYPE::efi_loader_data));
    map.entries = reinterpret_cast<kernel_memory_map_entry *>(
        allocate_pages(sizeof(kernel_memory_map_entry) * size, EFI_MEMORY_TYPE::reaveros_memory_map));

    size *= descriptor_size;

    switch (auto status = system_table->boot_services->get_memory_map(
                &size, map.get_efi_entry(0), &map.key, &descriptor_size, &descriptor_version))
    {
        case EFI_SUCCESS:
            break;

        default:
            console::print(u"[ERR] Error getting the memory map: ", status & ~high_bit, u".\n\r");
            halt();
    }

    size /= descriptor_size;

    map.size = size;

    console::print(u" > Actual size: ", size, u".\n\r");

    for (auto i = 0ull; i < size; ++i)
    {
        auto & efi_entry = *map.get_efi_entry(i);
        auto & entry = map.entries[i];

        entry.physical_start = efi_entry.physical_start;
        entry.length = efi_entry.number_of_pages * 4096;
        entry.attributes = 0; // TODO; forward attributes once needed

        switch (efi_entry.type)
        {
            case EFI_MEMORY_TYPE::efi_reserved_memory_type:
                entry.type = kernel_memory_type::unusable;
                break;
            case EFI_MEMORY_TYPE::efi_loader_code:
                entry.type = kernel_memory_type::loader;
                break;
            case EFI_MEMORY_TYPE::efi_loader_data:
                entry.type = kernel_memory_type::loader;
                break;
            case EFI_MEMORY_TYPE::efi_boot_services_code:
                entry.type = kernel_memory_type::free;
                break;
            case EFI_MEMORY_TYPE::efi_boot_services_data:
                entry.type = kernel_memory_type::free;
                break;
            case EFI_MEMORY_TYPE::efi_runtime_services_code:
                entry.type = kernel_memory_type::preserve;
                break;
            case EFI_MEMORY_TYPE::efi_runtime_services_data:
                entry.type = kernel_memory_type::preserve;
                break;
            case EFI_MEMORY_TYPE::efi_conventional_memory:
                entry.type = kernel_memory_type::free;
                break;
            case EFI_MEMORY_TYPE::efi_unusable_memory:
                entry.type = kernel_memory_type::unusable;
                break;
            case EFI_MEMORY_TYPE::efi_acpi_reclaim_memory:
                entry.type = kernel_memory_type::acpi_reclaimable;
                break;
            case EFI_MEMORY_TYPE::efi_acpi_memory_nvs:
                entry.type = kernel_memory_type::preserve;
                break;
            case EFI_MEMORY_TYPE::efi_memory_mapped_io:
                entry.type = kernel_memory_type::unusable;
                break;
            case EFI_MEMORY_TYPE::efi_memory_mapped_io_port_space:
                entry.type = kernel_memory_type::unusable;
                break;
            case EFI_MEMORY_TYPE::efi_pal_code:
                entry.type = kernel_memory_type::preserve;
                break;
            case EFI_MEMORY_TYPE::efi_persistent_memory:
                entry.type = kernel_memory_type::persistent;
                break;

            case EFI_MEMORY_TYPE::reaveros_kernel:
                entry.type = kernel_memory_type::kernel;
                break;
            case EFI_MEMORY_TYPE::reaveros_initrd:
                entry.type = kernel_memory_type::initrd;
                break;
            case EFI_MEMORY_TYPE::reaveros_paging:
                entry.type = kernel_memory_type::paging;
                break;
            case EFI_MEMORY_TYPE::reaveros_memory_map:
                entry.type = kernel_memory_type::memory_map;
                break;
            case EFI_MEMORY_TYPE::reaveros_backbuffer:
                entry.type = kernel_memory_type::backbuffer;
                break;

            default:
                entry.type = kernel_memory_type::unusable;
                console::print(
                    "[WRN] Unknown memory type found for entry at ",
                    reinterpret_cast<void *>(efi_entry.physical_start),
                    u": ",
                    static_cast<std::uint32_t>(efi_entry.type),
                    u", marking as unusable.\n\r");
                break;
        }
    }

    return map;
}

void exit_boot_services(const memory_map & map)
{
    system_table->boot_services->exit_boot_services(image_handle, map.key);
}
}

void * operator new(std::size_t size)
{
    if (size == 0)
    {
        return reinterpret_cast<void *>(~0ull);
    }

    void * ret = nullptr;
    switch (auto status = efi_loader::system_table->boot_services->allocate_pool(
                efi_loader::EFI_MEMORY_TYPE::efi_loader_data, size, &ret))
    {
        case efi_loader::EFI_SUCCESS:
            return ret;

        default:
            efi_loader::console::print(
                u"[ERR] Error allocating memory: ", status & ~efi_loader::high_bit, u".\n\r");
            efi_loader::halt();
    }
}

void * operator new[](std::size_t size)
{
    return operator new(size);
}

void operator delete(void * ptr) noexcept
{
    if (ptr == reinterpret_cast<void *>(~0ull))
    {
        return;
    }

    switch (auto status = efi_loader::system_table->boot_services->free_pool(ptr))
    {
        case efi_loader::EFI_SUCCESS:
            return;

        default:
            efi_loader::console::print(
                u"[ERR] Error freeing memory: ", status & ~efi_loader::high_bit, u".\n\r");
            efi_loader::halt();
    }
}

void operator delete[](void * ptr) noexcept
{
    operator delete(ptr);
}