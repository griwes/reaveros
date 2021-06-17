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

#include "arch/common/acpi/acpi.h"
#include "arch/cpu.h"
#include "boot/screen.h"
#include "memory/pmm.h"
#include "time/time.h"
#include "util/log.h"

#include <boot-arguments.h>
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

extern "C" void __cxa_atexit(void (*)(void *), void *, void *)
{
}

[[gnu::section(".reaveros_entry")]] extern "C" void kernel_main(boot_protocol::kernel_arguments args)
{
    kernel::boot_log::initialize(args.memory_map_size, args.memory_map_entries);

    if (args.has_video_mode)
    {
        kernel::boot_screen::initialize(args.video_mode, args.memory_map_size, args.memory_map_entries);
    }

    kernel::log::println("ReaverOS: Reaver Project Operating System, \"Rose\"");
    kernel::log::println(
        "Version: 0.0.5 dev; Release #1 \"Cotyledon\", built on {} at {}", __DATE__, __TIME__);
    kernel::log::println("Copyright (C) 2021 Reaver Project Team");
    kernel::log::println("");

    kernel::pmm::initialize(args.memory_map_size, args.memory_map_entries);
    kernel::pmm::report();

    kernel::acpi::initialize(args.acpi_revision, kernel::phys_addr_t{ args.acpi_root });
    kernel::arch::cpu::initialize();
    kernel::time::initialize();

    /*
    kernel::smp::boot();
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
