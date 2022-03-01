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

void bootinit_log_handler(
    kernel::ipc::mailbox * recv_mailbox,
    kernel::ipc::mailbox * ack_mailbox,
    kernel::scheduler::process * process,
    kernel::vm::vmo_mapping * mapping)
{
    kernel::util::intrusive_ptr bootinit_logging_recv_mailbox(recv_mailbox, kernel::util::adopt);
    kernel::util::intrusive_ptr bootinit_logging_ack_mailbox(ack_mailbox, kernel::util::adopt);
    kernel::util::intrusive_ptr bootinit_process(process, kernel::util::adopt);
    kernel::util::intrusive_ptr stack_mapping(mapping, kernel::util::adopt);

    while (true)
    {
        rose::syscall::mailbox_message msg{};

        auto result = [&]
        {
            kernel::util::interrupt_guard guard;
            return kernel::ipc::mailbox::syscall_rose_mailbox_read_handler(recv_mailbox, 0, &msg);
        }();

        if (result == rose::syscall::result::not_ready)
        {
            continue;
        }

        if (result != rose::syscall::result::ok)
        {
            PANIC("failed to receive a message from the bootinit logging mailbox!");
        }

        if (msg.type != rose::syscall::mailbox_message_type::user)
        {
            PANIC("bootinit logging mailbox contained a message of a wrong type!");
        }

        auto start = kernel::virt_addr_t(msg.payload.user.data0);
        auto end = start + msg.payload.user.data1;

        kernel::util::interrupt_guard guard;

        auto lock = bootinit_process->get_vas()->lock_address_range(start, end, false);

        if (!lock)
        {
            PANIC("bootinit logging mailbox contained a message pointing to an unmapped address!");
        }

        {
            std::lock_guard _(kernel::log::log_lock);
            auto it = kernel::boot_log::iterator();
            for (auto ptr = reinterpret_cast<char *>(start.value());
                 ptr != reinterpret_cast<char *>(end.value());
                 ++ptr)
            {
                *it++ = *ptr;
            }
        }

        msg = {};

        result = kernel::ipc::mailbox::syscall_rose_mailbox_write_handler(ack_mailbox, &msg);
        if (result != rose::syscall::result::ok)
        {
            PANIC("failed to send an ack message to the bootinit logging mailbox!");
        }
    }

    PANIC("TODO: implement thread termination");
}
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

        auto bootinit_vas_uptr = kernel::vm::create_vas(false);
        auto bootinit_vas = bootinit_vas_uptr.get();

        auto bootinit_process = kernel::scheduler::create_process(std::move(bootinit_vas_uptr));
        auto bootinit_thread = bootinit_process->create_thread();

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
            bootinit::addresses::top_of_stack - 31 * kernel::arch::vm::page_sizes[0],
            kernel::vm::flags::user);

        kernel::log::println(" > Creating the bootstrap mailboxes and sending handle tokens...");
        auto bootinit_mailbox = kernel::ipc::create_mailbox();

        kernel::log::println(" >> Creating and sending the logging mailbox and the ack mailbox...");
        auto bootinit_logging_mailbox = kernel::ipc::create_mailbox();
        auto blm_handle = kernel::create_handle(
            bootinit_logging_mailbox /* not moved on purpose */,
            kernel::permissions::write | kernel::permissions::transfer);
        bootinit_mailbox->send(std::move(blm_handle));

        auto bootinit_logging_ack_mailbox = kernel::ipc::create_mailbox();
        auto ack_handle = kernel::create_handle(
            bootinit_logging_ack_mailbox /* not moved on purpose */,
            kernel::permissions::read | kernel::permissions::transfer);
        bootinit_mailbox->send(std::move(ack_handle));

        kernel::log::println(" >> Sending kernel caps handle token...");
        bootinit_mailbox->send(kernel::create_handle(&kernel::kernel_caps));

        kernel::log::println(" >> Sending initrd VMO handle token...");
        bootinit_mailbox->send(kernel::create_handle(std::move(initrd_vmo)));

        kernel::log::println(" > Preparing bootinit context...");
        auto bm_handle = kernel::create_handle(std::move(bootinit_mailbox), kernel::permissions::read);
        bootinit_thread->get_context()->set_userspace();
        bootinit_thread->get_context()->set_instruction_pointer(bootinit::addresses::ip);
        bootinit_thread->get_context()->set_stack_pointer(bootinit::addresses::top_of_stack);
        bootinit_thread->get_context()->set_argument(
            0, bootinit_thread->get_container()->register_for_token(std::move(bm_handle)));

        kernel::log::println(" > Preparing initial log handler...");
        // TODO: abstraction for this, create_kernel_thread or something
        auto log_stack_addr = kernel::vm::allocate_address_range(32 * kernel::arch::vm::page_sizes[0]);
        auto log_stack_vmo = kernel::vm::create_sparse_vmo(31 * kernel::arch::vm::page_sizes[0]);
        log_stack_vmo->commit_all();
        auto log_stack_mapping = kernel::scheduler::get_kernel_process()->get_vas()->map_vmo(
            std::move(log_stack_vmo), log_stack_addr + kernel::arch::vm::page_sizes[0]);

        auto log_thread = bootinit_process->create_thread();
        log_thread->get_context()->set_instruction_pointer(
            kernel::virt_addr_t(reinterpret_cast<std::uintptr_t>(&bootinit_log_handler)));
        log_thread->get_context()->set_stack_pointer(
            log_stack_addr + 32 * kernel::arch::vm::page_sizes[0] - 8);
        log_thread->get_context()->set_argument(
            0, reinterpret_cast<std::uintptr_t>(bootinit_logging_mailbox.release(kernel::util::keep_count)));
        log_thread->get_context()->set_argument(
            1,
            reinterpret_cast<std::uintptr_t>(bootinit_logging_ack_mailbox.release(kernel::util::keep_count)));
        log_thread->get_context()->set_argument(
            2, reinterpret_cast<std::uintptr_t>(bootinit_process.release(kernel::util::keep_count)));
        log_thread->get_context()->set_argument(
            3, reinterpret_cast<std::uintptr_t>(log_stack_mapping.release(kernel::util::keep_count)));

        kernel::log::println(
            " > Scheduling bootinit threads. Kernel-side init done, servicing bootinit log messages.");

        {
            kernel::util::interrupt_guard guard;
            kernel::scheduler::schedule(std::move(log_thread));
            kernel::scheduler::schedule(std::move(bootinit_thread));
        }

        kernel::arch::cpu::idle();
    }
    ();
}
