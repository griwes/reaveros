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

#include "../include/rosert/init.h"

#include <rose/syscall/meta.h>

namespace __rosert::inline __v0
{
namespace
{
    std::uintptr_t init_mailbox = 0;

    rose::syscall::mailbox_user_message read_user_message(std::uintptr_t mailbox, std::size_t timeout = 0)
    {
        rose::syscall::mailbox_message message;

        auto result = rose::syscall::rose_mailbox_read(mailbox, timeout, &message);
        if (result != rose::syscall::result::ok || message.type != rose::syscall::mailbox_message_type::user)
        {
            // ... panic ...
            *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
        }

        return message.payload.user;
    }
}

using ctor_t = void (*)();
static_assert(sizeof(ctor_t) == 8);

void __init(std::uintptr_t mailbox)
{
    init_mailbox = mailbox;

    auto [init_count, _] = read_user_message(init_mailbox);

    while (init_count--)
    {
        auto [begin, end] = read_user_message(mailbox);
        for (auto ctor_i = begin; ctor_i < end; ctor_i += sizeof(ctor_t))
        {
            [[maybe_unused]] auto ctor = *reinterpret_cast<ctor_t *>(ctor_i);
            (*ctor)();
        }
    }
}
}
