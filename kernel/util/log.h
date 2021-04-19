/*
 * Copyright © 2021 Michał 'Griwes' Dominiak
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

#pragma once

#include "../boot/log_iterator.h"

#include <boot-memmap.h>

#include <format>
#include <string_view>

namespace kernel::log
{
void initialize(std::size_t memmap_size, boot_protocol::memory_map_entry * memmap);
void * get_syslog_mailbox();

template<typename... Ts>
void println(std::string_view fmt, const Ts &... args)
{
    if (get_syslog_mailbox())
    {
        // TODO
        *(volatile bool *)0 = false;
    }

    auto it = std::format_to(kernel::boot::log_iterator(), fmt, args...);
    *it = '\n';
}
}
