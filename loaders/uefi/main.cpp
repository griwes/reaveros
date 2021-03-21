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

#include "config/config.h"
#include "cpu/halt.h"
#include "cpu/detection.h"
#include "cpu/environment.h"
#include "efi/console.h"
#include "efi/efi.h"
#include "efi/filesystem.h"
#include "efi/memory_map.h"
#include "efi/system_table.h"
#include "efi/video_mode.h"

#include <cstring>

extern "C" efi_loader::EFI_STATUS efi_main(
    efi_loader::EFI_HANDLE image_handle,
    efi_loader::EFI_SYSTEM_TABLE * system_table)
{
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
        auto video_mode = efi_loader::choose_mode(config);

        if (video_mode.valid)
        {
            efi_loader::console::print(u"[GFX] Setting video mode...\n\r");
            efi_loader::set_mode(video_mode);
        }

        if (video_mode.valid)
        {
            efi_loader::allocate_pages(video_mode.info.framebuffer_size, 0x80001000);
        }

        efi_loader::console::print(u"[DSK] Loading kernel and initrd...\n\r");
        auto kernel = efi_loader::load_file(config["kernel"]);
        auto initrd = efi_loader::load_file(config["initrd"]);

        efi_loader::console::print(u"[MEM] Allocating memory regions...\n\r");
        auto kernel_region = efi_loader::allocate_pages(kernel.size, 0x80000000);
        auto initrd_region = efi_loader::allocate_pages(initrd.size, 0x80000001);

        std::memcpy(kernel_region, kernel.buffer.get(), kernel.size);
        std::memcpy(initrd_region, initrd.buffer.get(), initrd.size);

        efi_loader::console::print(u"[CPU] Preparing controlled execution environment structures...\n\r");
        efi_loader::prepare_environment();

        /*efi_loader::console::print(u"[MEM] Preparing paging structures...\n\r");
        if (video_mode.valid) {
        efi_loader::console::print(u"[GFX] Will set video mode.\n\r");
        efi_loader::allocate_pages(video buffer size, 0x80001000);
        }*/
    }
    /*efi_loader::console::print(
        u"[EFI] Bootloader done. Giving up EFI boot services and invoking the kernel.\n\r");*/

    *(volatile std::uint64_t *)nullptr = 0xdeadc0de;
    for (;;)
        ;
}
