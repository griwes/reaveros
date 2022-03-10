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

#include "log.h"

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

[[gnu::section(".bootinit_entry")]] extern "C" int bootinit_main(std::uintptr_t mailbox_token)
{
    __init();

    sc::mailbox_message message;

    auto result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok || message.type != sc::mailbox_message_type::handle_token)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    bootinit::log::logging_send_mailbox_token = message.payload.handle_token;

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok || message.type != sc::mailbox_message_type::handle_token)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    bootinit::log::logging_ack_mailbox_token = message.payload.handle_token;

    bootinit::log::println("[BOOT] Bootinit receiving initial handle tokens...");

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok)
    {
        PANIC("[ERR] Failed to read kernel caps token!");
    }

    if (message.type != sc::mailbox_message_type::handle_token)
    {
        PANIC("[ERR] Received wrong mailbox message type for kernel caps token!");
    }

    [[maybe_unused]] auto kernel_caps = message.payload.handle_token;
    bootinit::log::println(" > Kernel caps token received.");

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok)
    {
        PANIC("[ERR] Failed to read initrd VMO token!");
    }

    if (message.type != sc::mailbox_message_type::handle_token)
    {
        PANIC("[ERR] Receivfed wrong mailbox message type for initrd VMO token!");
    }

    [[maybe_unused]] auto initrd_vmo = message.payload.handle_token;
    bootinit::log::println(" > Initrd VMO token received.");

    bootinit::log::println("[BOOT] Done, spinning forever.");

    for (;;)
        ;
}
