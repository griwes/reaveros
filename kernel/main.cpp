/*
 * Copyright © 2021-2022 Michał 'Griwes' Dominiak
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
#include "arch/mp.h"
#include "boot/screen.h"
#include "bootinit/addresses.h"
#include "memory/pmm.h"
#include "memory/vmo.h"
#include "scheduler/mailbox.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread.h"
#include "time/time.h"
#include "util/log.h"
#include "util/mp.h"

#include <boot-arguments.h>
#include <boot-constants.h>
#include <boot-memmap.h>
#include <boot-video.h>

#include <cstddef>
#include <cstdint>

using ctor_t = void (*)();
extern "C" ctor_t __start_ctors;
extern "C" ctor_t __end_ctors;

extern "C" void __init()
{
    for (auto ctor = &__start_ctors; ctor != &__end_ctors; ++ctor)
    {
        (*ctor)();
    }
}

extern "C" void __cxa_atexit(void (*)(void *), void *, void *)
{
}

[[noreturn]] extern "C" void __cxa_pure_virtual()
{
    PANIC("Pure virtual method called!");
}

extern "C" char begin_bootinit[];
extern "C" char end_bootinit[];

extern "C" char begin_vdso[];
extern "C" char end_vdso[];

namespace
{
kernel::phys_addr_t initrd_base;
std::size_t initrd_size;
}

[[gnu::section(".reaveros_entry")]] extern "C" void kernel_main(boot_protocol::kernel_arguments args)
{
    __init();

    kernel::boot_log::initialize(args.memory_map_size, args.memory_map_entries);

    if (args.has_video_mode)
    {
        kernel::boot_screen::initialize(args.video_mode, args.memory_map_size, args.memory_map_entries);
    }

    kernel::log::println("ReaverOS: Reaver Project Operating System, \"Rose\"");
    kernel::log::println(
        "Version: 0.0.5 dev; Release #1 \"Cotyledon\", built on {} at {}", __DATE__, __TIME__);
    kernel::log::println("Copyright (C) 2021-2022 Reaver Project Team");
    kernel::log::println("");

    kernel::pmm::initialize(args.memory_map_size, args.memory_map_entries);
    kernel::pmm::report();

    kernel::acpi::initialize(args.acpi_revision, kernel::phys_addr_t{ args.acpi_root });
    kernel::arch::cpu::initialize();
    kernel::time::initialize();

    kernel::arch::mp::boot();
    kernel::mp::initialize_parallel();
    kernel::time::initialize_multicore();
    kernel::scheduler::initialize();

    auto initrd_entry = boot_protocol::find_entry(
        args.memory_map_size, args.memory_map_entries, boot_protocol::memory_type::initrd);
    if (!initrd_entry)
    {
        PANIC("Initrd not found!");
    }

    initrd_base = kernel::phys_addr_t(initrd_entry->physical_start);
    initrd_size = initrd_entry->length;

    kernel::arch::cpu::switch_to_clean_state();

    // IIFE to force the compiler to create a stack frame after a stack switch above
    // (otherwise it'd codegen assuming the stack frame of kernel_main is still valid,
    // even when this is not accessing any local variables)
    []() __attribute__((noinline))
    {
        kernel::log::println("[BOOT] Creating the bootinit process...");

        auto bootinit_vas = kernel::vm::create_vas(false);
        auto bootinit_process = kernel::scheduler::create_process(std::move(bootinit_vas));
        auto bootinit_thread = bootinit_process->create_thread();
        bootinit_process.release(kernel::util::drop_count);

        kernel::log::println(" > Creating and mapping the bootinit code VMO...");
        auto bootinit_code_vmo = kernel::vm::create_physical_vmo(
            kernel::arch::vm::virt_to_phys(
                kernel::virt_addr_t(reinterpret_cast<std::uintptr_t>(begin_bootinit))),
            end_bootinit - begin_bootinit);
        bootinit_vas->map_vmo(std::move(bootinit_code_vmo), bootinit::addresses::ip, kernel::vm::flags::user);

        kernel::log::println(" > Creating and mapping the VDSO VMO...");
        auto vdso_vmo = kernel::vm::create_physical_vmo(
            kernel::arch::vm::virt_to_phys(kernel::virt_addr_t(reinterpret_cast<std::uintptr_t>(begin_vdso))),
            end_vdso - begin_vdso);
        kernel::vm::set_vdso_vmo(vdso_vmo);
        bootinit_vas->map_vmo(std::move(vdso_vmo), bootinit::addresses::vdso, kernel::vm::flags::user);

        kernel::log::println(" > Creating and mapping the initrd VMO...");
        auto initrd_vmo = kernel::vm::create_physical_vmo(initrd_base, initrd_size);
        bootinit_vas->map_vmo(std::move(initrd_vmo), bootinit::addresses::initrd, kernel::vm::flags::user);

        kernel::log::println(" > Creating and mapping the bootinit stack VMO...");
        auto bootinit_stack_vmo = kernel::vm::create_sparse_vmo(31 * kernel::arch::vm::page_sizes[0]);
        // TODO: remove this once on-demand mappings are a thing
        bootinit_stack_vmo->commit_all();
        bootinit_vas->map_vmo(
            std::move(bootinit_stack_vmo),
            bootinit::addresses::top_of_stack - 31 * kernel::arch::vm::page_sizes[0]);

        auto kernel_process = kernel::scheduler::get_kernel_process().get();

        kernel::log::println(" > Creating the bootstrap mailbox and sending handle tokens...");
        auto bootinit_mailbox = kernel::ipc::create_mailbox();
        kernel::log::println(" >> Sending kernel caps handle token...");
        bootinit_mailbox->send(kernel::create_handle(kernel_process, &kernel::kernel_caps));
        kernel::log::println(" >> Sending initrd VMO handle token...");
        bootinit_mailbox->send(kernel::create_handle(kernel_process, std::move(initrd_vmo)));

        kernel::log::println(" > Preparing bootinit context...");
        bootinit_thread->get_context()->set_userspace();
        bootinit_thread->get_context()->set_instruction_pointer(bootinit::addresses::ip);
        bootinit_thread->get_context()->set_stack_pointer(bootinit::addresses::top_of_stack);
        bootinit_thread->get_context()->set_argument(
            0,
            kernel::create_handle(bootinit_thread->get_container(), std::move(bootinit_mailbox))
                ->get_token());

        bootinit_mailbox.release(kernel::util::drop_count);

        kernel::log::println(" > Posting the bootinit thread for scheduling. Kernel-side init done.");
        kernel::scheduler::post_schedule(std::move(bootinit_thread));

        kernel::arch::cpu::idle();
    }
    ();
}
