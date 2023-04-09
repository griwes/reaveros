/*
 * Copyright © 2022 Michał 'Griwes' Dominiak
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

#include "../../../kernel/bootinit/print.h"

#include <cstdint>

#include <rosert/init.h>

extern "C" [[noreturn]] void __cxa_pure_virtual()
{
    PANIC("Pure virtual method called!");
}

extern "C" void rose_main(std::uintptr_t inbox)
{
    rose::syscall::mailbox_message message;

    auto result = rose::syscall::rose_mailbox_read(inbox, -1, &message);
    if (result != rose::syscall::result::ok
        || message.type != rose::syscall::mailbox_message_type::handle_token)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    __rosert::__init(message.payload.handle_token);

    result = rose::syscall::rose_mailbox_read(inbox, 0, &message);
    if (result != rose::syscall::result::ok
        || message.type != rose::syscall::mailbox_message_type::handle_token)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    auto acceptor_mailbox_token = message.payload.handle_token;
    kernel_print::initialize(acceptor_mailbox_token);

    kernel_print::println("Hello from userspace logger!");

    for (;;)
        ;
}
