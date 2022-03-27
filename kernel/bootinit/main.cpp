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

#include "addresses.h"
#include "log.h"

#include <archive/cpio.h>

#include <user/meta.h>

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

namespace sc = rose::syscall;

namespace
{
std::uintptr_t kernel_caps_token;

struct mailbox_desc
{
    std::uintptr_t in;
    std::uintptr_t out;
};

struct create_process_result
{
    std::uintptr_t process_token;
    std::uintptr_t vas_token;
    std::uintptr_t protocol_mailbox_token;
};

create_process_result create_process(std::string_view)
{
    create_process_result ret;

    auto result = sc::rose_vas_create(kernel_caps_token, &ret.vas_token);
    if (result != sc::result::ok)
    {
        PANIC("failed to create a new virtual address space!");
    }

    return ret;
}
}

[[gnu::section(".bootinit_entry")]] extern "C" int bootinit_main(std::uintptr_t mailbox_token)
{
    __init();

    std::uintptr_t log_read_token;
    std::uintptr_t ack_write_token;

    auto result = sc::rose_mailbox_create(&log_read_token, &bootinit::log::logging_send_mailbox_token);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }
    result = sc::rose_mailbox_create(&bootinit::log::logging_ack_mailbox_token, &ack_write_token);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    sc::mailbox_message message;

    result = sc::rose_mailbox_read(mailbox_token, 0, &message);
    if (result != sc::result::ok || message.type != sc::mailbox_message_type::handle_token)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    auto accept_mailbox_token = message.payload.handle_token;

    message.type = sc::mailbox_message_type::handle_token;
    message.payload.handle_token = log_read_token;
    result = sc::rose_mailbox_write(accept_mailbox_token, &message);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    message.type = sc::mailbox_message_type::handle_token;
    message.payload.handle_token = ack_write_token;
    result = sc::rose_mailbox_write(accept_mailbox_token, &message);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    bootinit::log::println("Receiving initial handle tokens...");

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok)
    {
        PANIC("Failed to read kernel caps token!");
    }

    if (message.type != sc::mailbox_message_type::handle_token)
    {
        PANIC("Received wrong mailbox message type for kernel caps token!");
    }

    kernel_caps_token = message.payload.handle_token;
    bootinit::log::println(" > Kernel caps token received.");

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok)
    {
        PANIC("Failed to read initrd VMO token!");
    }

    if (message.type != sc::mailbox_message_type::handle_token)
    {
        PANIC("Received wrong mailbox message type for initrd VMO token!");
    }

    [[maybe_unused]] auto initrd_vmo = message.payload.handle_token;
    bootinit::log::println(" > Initrd VMO token received.");

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok)
    {
        PANIC("Failed to read initrd size!");
    }

    if (message.type != sc::mailbox_message_type::user)
    {
        PANIC("Received wrong mailbox message type for initrd size!");
    }

    auto initrd_size = message.payload.user.data0;

    bootinit::log::println(" > Initrd size: {} bytes.", initrd_size);

    bootinit::log::println("Parsing initrd image...");

    auto initrd_result =
        archive::try_cpio(reinterpret_cast<char *>(bootinit::addresses::initrd.value()), initrd_size);

    if (!initrd_result.archive)
    {
        PANIC(
            "The initrd image is not a valid cpio archive! Reason: {}, header offset: {}.",
            initrd_result.error_message,
            initrd_result.header_offset);
    }

    auto & initrd = *initrd_result.archive;

    bootinit::log::println(" > Number of regular files in initrd: {}.", initrd.size());

    auto test_file = initrd["test-file"];
    if (!test_file)
    {
        PANIC("'test-file not found!");
    }
    bootinit::log::println(" > Contents of test-file: '{}'.", *test_file);

    bootinit::log::println("Creating vasmgr process...");

    auto vasmgr_image = initrd["system/vasmgr.srv"];
    if (!vasmgr_image)
    {
        PANIC("system/vasmgr.srv not found in the initrd image!");
    }

    [[maybe_unused]] auto [vasmgr_process, vasmgr_vas, vasmgr_protocol_mailbox] =
        create_process(*vasmgr_image);

    /*
    auto procmgr_image = initrd["system/procmgr.srv"];
    if (!procmgr_image)
    {
        PANIC("system/procmgr.srv not found in the initrd image!");
    }

    auto [procmgr_process, procmgr_vas, procmgr_protocol_mailbox] = create_process(*procmgr_image);

    auto broker_image = initrd["system/broker.srv"];
    if (!broker_image)
    {
        PANIC("system/broker.srv not found in the initrd image!");
    }

    auto [broker_process, broker_vas, broker_protocol_mailbox] = create_process(*broker_image);
    */

    for (;;)
        ;
}
