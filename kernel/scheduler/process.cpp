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

#include "process.h"
#include "../util/interrupt_control.h"
#include "thread.h"

namespace kernel::scheduler
{
process::process(util::intrusive_ptr<vm::vas> address_space) : _address_space(std::move(address_space))
{
}

handle_token_t process::register_for_token(util::intrusive_ptr<handle> hnd)
{
    auto new_element = std::make_unique<_handle_store>();
    new_element->handle = std::move(hnd);

    handle_token_t token;

    util::interrupt_guard guard;
    std::lock_guard _(_lock);

    while (true)
    {
        auto self_uint = reinterpret_cast<std::uintptr_t>(this);
        auto handle_uint = reinterpret_cast<std::uintptr_t>(new_element->handle.get());
        auto timestamp = time::get_high_precision_timer().now().time_since_epoch().count();

        token = handle_token_t(self_uint ^ handle_uint ^ timestamp);

        if (_handles.find(token) == _handles.end())
        {
            break;
        }
    }

    new_element->token = token;
    auto result = _handles.insert(std::move(new_element));
    if (!result.second)
    {
        PANIC("failed to insert a handle into handles tree!");
    }

    return token;
}

void process::unregister_token(handle_token_t token)
{
    util::interrupt_guard guard;
    std::lock_guard _(_lock);

    auto it = _handles.find(token);
    if (it == _handles.end())
    {
        PANIC("tried to unregister a token that doesn't exit");
    }
    _handles.erase(it);
}

util::intrusive_ptr<handle> process::get_handle(handle_token_t token) const
{
    util::interrupt_guard guard;
    std::lock_guard _(_lock);

    auto it = _handles.find(token);
    if (it == _handles.end())
    {
        return {};
    }

    return it->handle;
}

util::intrusive_ptr<thread> process::create_thread()
{
    auto ret = util::make_intrusive<thread>(util::intrusive_ptr<process>(this));

    ret->timestamp = time::get_high_precision_timer().now();

    return ret;
}

rose::syscall::result process::syscall_rose_token_release_handler(std::uintptr_t token)
{
    if (token == 0)
    {
        return rose::syscall::result::ok;
    }

    auto process = arch::cpu::get_core_local_storage()->current_thread->get_container();
    process->unregister_token(handle_token_t(token)); // TODO: propagate errors instead of a panic

    return rose::syscall::result::ok;
}
}
