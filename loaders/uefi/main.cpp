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

#include "config/config.h"
#include "cpu/detection.h"
#include "cpu/environment.h"
#include "cpu/halt.h"
#include "cpu/paging.h"
#include "efi/console.h"
#include "efi/efi.h"
#include "efi/filesystem.h"
#include "efi/system_table.h"
#include "efi/video_mode.h"

#include <boot-arguments.h>
#include <boot-constants.h>

#include <cstring>

extern "C" efi_loader::EFI_STATUS efi_main(
    efi_loader::EFI_HANDLE image_handle,
    efi_loader::EFI_SYSTEM_TABLE * system_table)
{
    efi_loader::video_mode video_mode;

    {
        if (system_table->header.signature != efi_loader::EFI_SYSTEM_TABLE_SIGNATURE)
        {
            // need a better way to handle this
            // probably also need a crc32 check, but fuck that right now
            *(volatile std::uint64_t *)nullptr = 0xdeadbeef;
            efi_loader::halt();
        }

        efi_loader::initialize(system_table, image_handle);
        efi_loader::console::initialize();

        efi_loader::console::print(u"ReaverOS UEFI bootloader\n\r");
        efi_loader::console::print(u"Version 0.0.0 alpha, codename \"Cotyledon\"\n\r");
        efi_loader::console::print(u"Copyright (C) 2021 Reaver Project Team\n\n\r");

        efi_loader::console::print(u"[CPU] Checking CPU capabilities...\n\r");
        auto cpu_caps = efi_loader::detect_cpu();

        efi_loader::console::print(u"[EFI] Looking for own location...\n\r");
        auto source_directory = efi_loader::locate_source_directory(image_handle);

        efi_loader::console::print(u"[DSK] Loading configuration...\n\r");
        auto config = efi_loader::config{ efi_loader::load_file(source_directory / u"reaveros.conf") };

        efi_loader::console::print(u"[GFX] Choosing video mode...\n\r");
        video_mode = efi_loader::choose_mode(config);

        if (video_mode.valid)
        {
            efi_loader::console::print(u"[GFX] Setting video mode...\n\r");
            efi_loader::set_mode(video_mode);
        }

        void * backbuffer_region = nullptr;

        if (video_mode.valid)
        {
            backbuffer_region = efi_loader::allocate_pages(
                video_mode.info.framebuffer_size, efi_loader::EFI_MEMORY_TYPE::reaveros_backbuffer);

            efi_loader::console::print(u" > Video backbuffer physical address: ", backbuffer_region, u"\n\r");
        }

        efi_loader::console::print(u"[DSK] Loading kernel and initrd...\n\r");
        auto kernel = efi_loader::load_file(config["kernel"]);
        auto initrd = efi_loader::load_file(config["initrd"]);

        efi_loader::console::print(u"[MEM] Allocating memory regions...\n\r");
        auto kernel_region =
            efi_loader::allocate_pages(kernel.size, efi_loader::EFI_MEMORY_TYPE::reaveros_kernel);
        auto initrd_region =
            efi_loader::allocate_pages(initrd.size, efi_loader::EFI_MEMORY_TYPE::reaveros_initrd);
        auto early_log_buffer =
            efi_loader::allocate_pages(2 * 1024 * 1024, efi_loader::EFI_MEMORY_TYPE::reaveros_log_buffer);

        efi_loader::console::print(u" > Kernel physical address: ", kernel_region, u"\n\r");
        efi_loader::console::print(u" > Initrd physical address: ", initrd_region, u"\n\r");
        efi_loader::console::print(u" > Early log buffer physical address: ", early_log_buffer, u"\n\r");

        std::memcpy(kernel_region, kernel.buffer.get(), kernel.size);
        std::memcpy(initrd_region, initrd.buffer.get(), initrd.size);

        efi_loader::console::print(u"[MEM] Preparing paging structures...\n\r");
        efi_loader::prepare_paging();

        auto image_info = efi_loader::get_image_info();

        efi_loader::console::print(u" > Identity mapping the loader code...\n\r");
        efi_loader::vm_map(
            image_info.image_base,
            image_info.image_size,
            reinterpret_cast<std::uintptr_t>(image_info.image_base));

        efi_loader::console::print(u" > Identity mapping the loader stack...\n\r");
        auto approximately_stack = reinterpret_cast<std::uint8_t *>(&image_handle);
        efi_loader::vm_map(
            approximately_stack - 2 * 4096,
            3 * 4096,
            reinterpret_cast<std::uintptr_t>(approximately_stack - 2 * 4096));

        efi_loader::console::print(
            u" > Mapping the kernel at ", reinterpret_cast<void *>(boot_protocol::kernel_base), u"...\n\r");
        efi_loader::vm_map(kernel_region, kernel.size, boot_protocol::kernel_base);

        efi_loader::console::print(
            u" > Mapping supported physical address space at ",
            reinterpret_cast<void *>(boot_protocol::physmem_base),
            u"...\n\r");
        if (cpu_caps.huge_pages_supported)
        {
            efi_loader::vm_map_huge(
                nullptr, 1ull << cpu_caps.physical_address_bits, boot_protocol::physmem_base);
        }
        else
        {
            efi_loader::vm_map_large(
                nullptr, 1ull << cpu_caps.physical_address_bits, boot_protocol::physmem_base);
        }
    }

    efi_loader::console::print(u"[EFI] Locating the ACPI root...\n\r");
    auto acpi_info = efi_loader::find_acpi_root();

    efi_loader::console::print(u"[EFI] Retrieving the memory map...\n\r");
    auto memmap = efi_loader::get_memory_map();

    auto approximately_stack = reinterpret_cast<std::uintptr_t>(&image_handle);
    memmap.account_for_stack(approximately_stack, approximately_stack + 3 * 4096);

    efi_loader::console::print(u"[EFI] Loader done, giving up boot services and invoking kernel.\n\r");

    efi_loader::exit_boot_services(memmap);
    efi_loader::prepare_environment();

    auto memmap_entries_remapped = reinterpret_cast<boot_protocol::memory_map_entry *>(
        boot_protocol::physmem_base + reinterpret_cast<std::uintptr_t>(memmap.entries));

    boot_protocol::kernel_arguments args{};

    args.memory_map_size = memmap.size;
    args.memory_map_entries = memmap_entries_remapped;
    args.has_video_mode = video_mode.valid;
    args.video_mode = &video_mode.info;

    args.acpi_revision = acpi_info.revision;
    args.acpi_root = acpi_info.root;

    using kernel_entry_t = void __attribute__((sysv_abi)) (*)(boot_protocol::kernel_arguments);
    auto kernel_entry = reinterpret_cast<kernel_entry_t>(boot_protocol::kernel_base);
    kernel_entry(args);

    __builtin_unreachable();
}
