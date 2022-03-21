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
#include "../util/interrupt_control.h"
#include "scheduler.h"
#include "thread.h"

namespace kernel::ipc
{
util::intrusive_ptr<mailbox> create_mailbox()
{
    auto ret = util::make_intrusive<mailbox>();

    return ret;
}

void mailbox::_push(std::unique_ptr<mailbox_message> message)
{
    util::interrupt_guard guard;
    std::lock_guard _(_lock);

    _message_queue.push_back(std::move(message));

    if (!_waiting_threads.empty())
    {
        auto thread = _waiting_threads.pop_front();
        scheduler::schedule(std::move(thread));
    }
}

void mailbox::send(util::intrusive_ptr<handle> handle)
{
    _push(std::make_unique<mailbox_message>(mailbox_message{ .payload = std::move(handle) }));
}

void mailbox::send(rose::syscall::mailbox_user_message message)
{
    _push(std::make_unique<mailbox_message>(mailbox_message{ .payload = message }));
}

rose::syscall::result mailbox::syscall_rose_mailbox_create_handler(
    std::uintptr_t * read_token,
    std::uintptr_t * write_token)
{
    auto current_thread = arch::cpu::get_core_local_storage()->current_thread;

    auto mb = create_mailbox();

    auto read_handle = create_handle(mb, permissions::read | permissions::transfer | permissions::clone);
    *read_token = current_thread->get_container()->register_for_token(std::move(read_handle)).value();

    auto write_handle =
        create_handle(std::move(mb), permissions::write | permissions::transfer | permissions::clone);
    *write_token = current_thread->get_container()->register_for_token(std::move(write_handle)).value();

    return rose::syscall::result::ok;
}

std::optional<rose::syscall::result> mailbox::syscall_rose_mailbox_read_handler(
    mailbox * mb,
    std::uintptr_t timeout,
    rose::syscall::mailbox_message * target)
{
    util::interrupt_guard guard;
    std::unique_lock _(mb->_lock);

    if (timeout != static_cast<std::uintptr_t>(-1) && timeout != 0)
    {
        PANIC("TODO: support with mailbox read with a specified timeout");
    }

    if (mb->_message_queue.empty())
    {
        if (timeout == static_cast<std::uintptr_t>(-1))
        {
            return rose::syscall::result::not_ready;
        }

        auto cls = arch::cpu::get_core_local_storage();
        mb->_waiting_threads.push_back(cls->current_core->get_scheduler()->deschedule());

        return std::nullopt;
    }

    auto message = mb->_message_queue.pop_front();

    switch (message->payload.index())
    {
        case 0:
        {
            target->type = rose::syscall::mailbox_message_type::handle_token;

            auto current_thread = arch::cpu::get_core_local_storage()->current_thread;
            target->payload.handle_token = current_thread->get_container()
                                               ->register_for_token(std::move(std::get<0>(message->payload)))
                                               .value();

            break;
        }

        case 1:
        {
            target->type = rose::syscall::mailbox_message_type::user;
            target->payload.user = std::get<1>(message->payload);

            break;
        }

        default:
        {
            PANIC("rose_mailbox_read from a malbox containing an unimplemented payload type");
        }
    }

    return rose::syscall::result::ok;
}

rose::syscall::result mailbox::syscall_rose_mailbox_write_handler(
    mailbox * mb,
    const rose::syscall::mailbox_message * source)
{
    util::interrupt_guard guard;
    std::lock_guard _(mb->_lock);

    switch (source->type)
    {
        case rose::syscall::mailbox_message_type::handle_token:
        {
            auto current_process = arch::cpu::get_core_local_storage()->current_thread->get_container();
            auto token = handle_token_t(source->payload.handle_token);
            auto handle = current_process->get_handle(token);

            if (!handle)
            {
                return rose::syscall::result::invalid_token;
            }

            if (!handle->has_permissions(permissions::transfer))
            {
                return rose::syscall::result::not_allowed;
            }

            current_process->unregister_token(token);

            mb->_message_queue.push_back(
                std::make_unique<mailbox_message>(mailbox_message{ .payload = std::move(handle) }));
            break;
        }

        case rose::syscall::mailbox_message_type::user:
        {
            mb->_message_queue.push_back(
                std::make_unique<mailbox_message>(mailbox_message{ .payload = source->payload.user }));
            break;
        }

        default:
        {
            PANIC("rose_mailbox_write with a message containing an unimplemented payload type");
        }
    }

    if (!mb->_waiting_threads.empty())
    {
        auto thread = mb->_waiting_threads.pop_front();
        scheduler::schedule(std::move(thread));
    }

    return rose::syscall::result::ok;
}
}
