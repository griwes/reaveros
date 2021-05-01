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

#include <boot-memmap.h>

#include <format>
#include <string_view>

// TODO: sane abstraction for this
#define PANIC(...)                                                                                           \
    kernel::log::println("PANIC: " __VA_ARGS__);                                                             \
    asm volatile("cli; hlt;")

namespace kernel::boot_log
{
struct buffer_chunk;

void initialize(std::size_t memmap_size, boot_protocol::memory_map_entry * memmap);

class iterator
{
public:
    class proxy
    {
    public:
        proxy() = default;

        proxy(const proxy &) = delete;
        proxy & operator=(const proxy &) = delete;

        const proxy & operator=(char c) const;
    };

    using iterator_category = std::output_iterator_tag;
    using value_type = char;
    using difference_type = std::ptrdiff_t;
    using pointer = char *;
    using reference = proxy;

    iterator();
    ~iterator();

    iterator(const iterator &) = default;
    iterator(iterator &&) = default;
    iterator & operator=(const iterator &) = default;
    iterator & operator=(iterator &&) = default;

    proxy operator*() const
    {
        return {};
    }

    iterator & operator++()
    {
        return *this;
    }

    iterator operator++(int)
    {
        return *this;
    }
};
}

namespace kernel::log
{
void * get_syslog_mailbox();

template<typename... Ts>
void println(std::string_view fmt, const Ts &... args)
{
    if (get_syslog_mailbox())
    {
        // TODO
        *(volatile bool *)0 = false;
    }

    auto it = std::format_to(kernel::boot_log::iterator(), fmt, args...);
    *it = '\n';
}
}
