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

#include "../../../include/format"

#include <cassert>
#include <string_view>

template<typename T>
void test_value(T value, std::string_view str, __ROSESTD::__format_string<T> fmt)
{
    char buffer[100] = "";
    __ROSESTD::format_to(+buffer, fmt, value);

    assert(str == buffer);
}

template<typename T>
void test()
{
    if constexpr (std::is_same_v<T, char>)
    {
        test_value<T>('a', "a", "{}");
        test_value<T>('b', "b", "{:c}");
    }
    else
    {
        test_value<T>(17, "17", "{}");
        test_value<T>(0, "0", "{}");
        if constexpr (__is_signed(T))
        {
            test_value<T>(-29, "-29", "{}");
        }
    }

    test_value<T>(17, "17", "{:d}");
    test_value<T>(0, "0", "{:d}");

    test_value<T>(0b01011011, "1011011", "{:b}");
    test_value<T>(0b01011011, "0b1011011", "{:#b}");

    test_value<T>(0b01011011, "1011011", "{:B}");
    test_value<T>(0b01011011, "0B1011011", "{:#B}");

    test_value<T>(0x7b, "7b", "{:x}");
    test_value<T>(0x7b, "0x7b", "{:#x}");

    test_value<T>(0x7b, "7B", "{:X}");
    test_value<T>(0x7b, "0X7B", "{:#X}");

    test_value<T>(0160, "160", "{:o}");
    test_value<T>(0160, "0160", "{:#o}");

    if constexpr (sizeof(T) * CHAR_BIT > 32)
    {
        test_value<T>(0x1213141516, "0x1213141516", "{:#x}");
    }

    test_value<T>(17, "+17", "{:+d}");
    test_value<T>(0, "+0", "{:+d}");

    test_value<T>(0b01011011, "+1011011", "{:+b}");
    test_value<T>(0b01011011, " 0b1011011", "{: #b}");

    test_value<T>(0b01011011, "+1011011", "{:+B}");
    test_value<T>(0b01011011, "+0B1011011", "{:+#B}");

    test_value<T>(0x7b, " 7b", "{: x}");
    test_value<T>(0x7b, "+0x7b", "{:+#x}");

    test_value<T>(0x7b, "+7B", "{:+X}");
    test_value<T>(0x7b, "+0X7B", "{:+#X}");

    test_value<T>(0160, "+160", "{:+o}");
    test_value<T>(0160, " 0160", "{: #o}");

    if constexpr (sizeof(T) * CHAR_BIT > 32)
    {
        test_value<T>(0x1213141516, "+0x1213141516", "{:+#x}");
    }

    if constexpr (__is_signed(T))
    {
        test_value<T>(-17, "-17", "{:d}");

        test_value<T>(-0b01011011, "-1011011", "{:b}");
        test_value<T>(-0b01011011, "-0b1011011", "{:#b}");

        test_value<T>(-0b01011011, "-1011011", "{:B}");
        test_value<T>(-0b01011011, "-0B1011011", "{:#B}");

        test_value<T>(-0x7b, "-7b", "{:x}");
        test_value<T>(-0x7b, "-0x7b", "{:#x}");

        test_value<T>(-0x7b, "-7B", "{:X}");
        test_value<T>(-0x7b, "-0X7B", "{:#X}");

        test_value<T>(-0160, "-160", "{:o}");
        test_value<T>(-0160, "-0160", "{:#o}");

        if constexpr (sizeof(T) * CHAR_BIT > 32)
        {
            test_value<T>(-0x1213141516, "-0x1213141516", "{:#x}");
        }
    }

    test_value<T>(17, "  17", "{:4}");
    test_value<T>(17, "0017", "{:04}");
    test_value<T>(17, "17  ", "{:<04}");

    test_value<T>(0x1a, "  0x1a", "{:#6x}");
    test_value<T>(0x1a, "0x001a", "{:#06x}");
    test_value<T>(0x1a, "0X001A", "{:#06X}");
    test_value<T>(0x1a, "  0x1a", "{:>#06x}");

    test_value<T>(0127, "000127", "{:06o}");
    test_value<T>(0127, " 127  ", "{:^06o}");
}

int main()
{
    // test<char>();
    test<signed char>();
    test<unsigned char>();
    test<signed short>();
    test<unsigned short>();
    test<signed int>();
    test<unsigned int>();
    test<signed long>();
    test<unsigned long>();
    test<signed long long>();
    test<unsigned long long>();
    test<__ROSESTD::int128_t>();
    test<__ROSESTD::uint128_t>();
}
