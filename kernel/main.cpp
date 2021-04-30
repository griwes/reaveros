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

#include "boot/screen.h"
#include "util/log.h"

#include <boot-constants.h>
#include <boot-memmap.h>
#include <boot-video.h>

#include <cstddef>
#include <cstdint>

using ctor_t = void (*)();
extern "C" ctor_t __start_ctors;
extern "C" ctor_t __end_ctors;

void __init()
{
    for (auto ctor = &__start_ctors; ctor != &__end_ctors; ++ctor)
    {
        (*ctor)();
    }
}

[[gnu::section(".reaveros_entry")]] extern "C" void kernel_main(
    std::size_t memmap_size,
    boot_protocol::memory_map_entry * memmap_entries,
    bool has_video_mode,
    boot_protocol::video_mode * video_mode)
{
    kernel::boot_log::initialize(memmap_size, memmap_entries);

    if (has_video_mode)
    {
        kernel::boot_screen::initialize(video_mode, memmap_size, memmap_entries);
    }

    kernel::log::println("ReaverOS: Reaver Project Operating System, \"Rose\"");
    kernel::log::println(
        "Version: 0.0.5 dev; Release #1 \"Cotyledon\", built on {} at {}", __DATE__, __TIME__);
    kernel::log::println("Copyright (C) 2021 Reaver Project Team");
    kernel::log::println("");

    /*
    kernel::pmm::initialize(memmap_size, memmap_entries);
    kernel::vas::initialize();

    kernel::pmm::boot_report();

    kernel::cpu::initialize();

    kernel::scheduler::initialize();

    // find initrd
    // find the boot init file in the initrd
    // clone the VAS
    // map the boot init file in the clone
    // map the initrd in the clone
    // jump to userspace at the boot init address
    */

    while (true)
    {
        asm volatile("hlt");
    }
}
