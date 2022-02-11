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

#include <user/meta.h>

namespace sc = rose::syscall;

std::uintptr_t logging_mailbox_token;

[[gnu::section(".bootinit_entry")]] extern "C" int bootinit_main(std::uintptr_t mailbox_token)
{
    sc::mailbox_message message;

    auto result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok || message.type != sc::mailbox_message_type::handle_token)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    logging_mailbox_token = message.payload.handle_token;

    /*
    log::println("[BOOT] Bootinit receiving initial handle tokens...");

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok)
    {
        PANIC("[ERR] Failed to read kernel caps token!");
    }

    if (message.type != sc::mailbox_message_type::handle_token)
    {
        PANIC("[ERR] Received wrong mailbox message type for kernel caps token!");
    }

    auto kernel_caps = message.payload.handle_token;
    log::println(" > Kernel caps token received.");

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok)
    {
        PANIC("[ERR] Failed to read initrd VMO token!");
    }

    if (message.type != sc::mailbox_message_types::handle_token)
    {
        PANIC("[ERR] Receivfed wrong mailbox message type for initrd VMO token!");
    }

    auto initrd_vmo = message.payload.handle_token;
    log::println(" > Initrd VMO token received.");
    */

    for (;;)
        ;
}
