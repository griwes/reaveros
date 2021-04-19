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

#include <cstdint>
#include <iterator>

namespace kernel::boot
{
static std::size_t port = 0x3F8;

class log_iterator
{
public:
    struct proxy
    {
        static inline void outb(std::uint16_t port, std::uint8_t value)
        {
            asm volatile("outb %1, %0" ::"dN"(port), "a"(value));
        }

        const proxy & operator=(char c) const
        {
            outb(port, c);
            return *this;
        }
    };

    using iterator_category = std::output_iterator_tag;
    using value_type = char;
    using difference_type = std::ptrdiff_t;
    using pointer = char *;
    using reference = proxy;

    log_iterator()
    {
    }

    log_iterator(const log_iterator &) = default;
    log_iterator(log_iterator &&) = default;
    log_iterator & operator=(const log_iterator &) = default;
    log_iterator & operator=(log_iterator &&) = default;

    proxy operator*() const
    {
        return {};
    }

    log_iterator & operator++()
    {
        return *this;
    }

    log_iterator operator++(int)
    {
        return *this;
    }
};
}
