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

#include "print.h"

#include <mutex>

namespace sc = rose::syscall;

namespace kernel_print
{
std::uintptr_t logging_send_mailbox_token;
std::uintptr_t logging_ack_mailbox_token;
std::mutex log_lock;

namespace
{
    char log_buffer[4096];
    char * log_cursor = nullptr;
}

void initialize(std::uintptr_t acceptor_mailbox_token)
{
    std::uintptr_t log_read_token;
    std::uintptr_t ack_write_token;

    auto result = sc::rose_mailbox_create(&log_read_token, &logging_send_mailbox_token);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }
    result = sc::rose_mailbox_create(&logging_ack_mailbox_token, &ack_write_token);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    sc::mailbox_message message{};

    message.type = sc::mailbox_message_type::handle_token;
    message.payload.handle_token = log_read_token;
    result = sc::rose_mailbox_write(acceptor_mailbox_token, &message);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    message.type = sc::mailbox_message_type::handle_token;
    message.payload.handle_token = ack_write_token;
    result = sc::rose_mailbox_write(acceptor_mailbox_token, &message);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }
}

const iterator::proxy & iterator::proxy::operator=(char c) const
{
    if (!log_cursor) [[unlikely]]
    {
        log_cursor = log_buffer;
    }

    *log_cursor++ = c;

    if (log_cursor == std::end(log_buffer))
    {
        flush();
    }

    return *this;
}

void flush()
{
    sc::mailbox_message msg{};
    msg.type = sc::mailbox_message_type::user;

    msg.payload.user.data0 = reinterpret_cast<std::uintptr_t>(log_buffer);
    msg.payload.user.data1 = log_cursor - log_buffer;

    auto result = sc::rose_mailbox_write(logging_send_mailbox_token, &msg);
    if (result != sc::result::ok)
    {
        *reinterpret_cast<volatile std::uintptr_t *>(0) = std::to_underlying(result);
    }

    result = sc::rose_mailbox_read(logging_ack_mailbox_token, 0, &msg);
    if (result != sc::result::ok)
    {
        *reinterpret_cast<volatile std::uintptr_t *>(1) = std::to_underlying(result);
    }

    log_cursor = log_buffer;
}
}
