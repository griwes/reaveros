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

#include "mailbox.h"
#include "../arch/cpu.h"
#include "thread.h"

namespace kernel::ipc
{
util::intrusive_ptr<mailbox> create_mailbox()
{
    auto ret = util::make_intrusive<mailbox>();

    return ret;
}

void mailbox::send(util::intrusive_ptr<handle> handle)
{
    auto message = std::make_unique<mailbox_message>(mailbox_message{ .payload = std::move(handle) });

    std::lock_guard _(_lock);

    _message_queue.push_back(std::move(message));

    if (!_waiting_threads.empty())
    {
        PANIC("TODO: implement waking up threads sleeping on a mailbox");
    }
}

// syscalls
rose::syscall::result mailbox::syscall_rose_mailbox_read_handler(
    mailbox * mb,
    std::uintptr_t timeout,
    rose::syscall::mailbox_message * target)
{
    std::lock_guard _(mb->_lock);

    if (timeout != static_cast<std::uintptr_t>(-1))
    {
        PANIC("TODO: support with mailbox read with a timeout");
    }

    if (mb->_message_queue.empty())
    {
        return rose::syscall::result::not_ready;
    }

    auto message = mb->_message_queue.pop_front();

    target->type = rose::syscall::mailbox_message_type::handle_token;

    auto current_thread = arch::cpu::get_core_local_storage()->current_thread;
    target->payload.handle_token =
        current_thread->get_container()->register_for_token(std::move(message->payload)).value();

    return rose::syscall::result::ok;
}

rose::syscall::result mailbox::syscall_rose_mailbox_write_handler(
    mailbox *,
    const rose::syscall::mailbox_message *)
{
    PANIC("got asked to write a message to a mailbox!");
}
}
