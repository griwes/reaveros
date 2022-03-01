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

#include <user/meta.h>

#include <format>
#include <iterator>
#include <mutex>

#define PANIC(...)                                                                                           \
    bootinit::log::println("PANIC: " __VA_ARGS__);                                                           \
    for (;;)                                                                                                 \
        ;                                                                                                    \
    __builtin_unreachable()

namespace bootinit::log
{
extern std::uintptr_t logging_send_mailbox_token;
extern std::uintptr_t logging_ack_mailbox_token;

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

    iterator() = default;

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

extern std::mutex log_lock;

void flush();

template<typename... Ts>
void println(std::__format_string<Ts...> fmt, const Ts &... args)
{
    std::lock_guard _(log_lock);

    if (logging_send_mailbox_token == 0)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    auto it = std::format_to(iterator(), fmt, args...);
    *it = '\n';
    flush();
}
}
