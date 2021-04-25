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

#include <climits>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

namespace efi_loader
{
namespace console
{
    void initialize();
    void clear();

    void print(const char *);
    void print(const char16_t *);
    void print(std::string_view);
    void print(void *);

    template<typename T>
    void print(const T & a) requires std::integral<T>
    {
        static_assert(
            sizeof(T) * CHAR_BIT <= 64,
            "a bigger-than-64-bit type needs more than 23 characters of a buffer");

        char16_t buffer[23];
        auto buffer_ptr = buffer;

        if (a == 0)
        {
            buffer[0] = u'0';
            buffer[1] = u'\0';
            print(buffer);
            return;
        }

        if constexpr (std::is_signed<T>::value)
        {
            if (!(a > 0 || a == 0))
            {
                *buffer_ptr++ = u'-';
            }
        }

        auto impl = [&buffer_ptr](auto && self, auto a) -> void
        {
            T div = a / 10;
            T mod = a % 10;

            if (div != 0)
            {
                self(self, div);
            }

            *buffer_ptr++ = u'0' + mod;
        };

        impl(impl, a);
        *buffer_ptr = u'\0';
        print(buffer);
    }

    inline void print(char * str)
    {
        print(static_cast<const char *>(str));
    }

    inline void print(char16_t * str)
    {
        print(static_cast<const char16_t *>(str));
    }

    template<typename T>
    void print(T * ptr)
    {
        print(reinterpret_cast<void *>(ptr));
    }

    template<typename T>
    requires requires(T && t)
    {
        { std::forward<T>(t).print() };
    }
    void print(T && t)
    {
        std::forward<T>(t).print();
    }

    template<typename... Ts>
    void print(Ts &&... ts) requires(sizeof...(Ts) > 1)
    {
        using swallow = int[];
        (void)swallow{ (print(std::forward<Ts>(ts)), 0)... };
    }
}
}
