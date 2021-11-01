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

#include "../../../include/charconv"
#include "../../../include/cstdint"

#include <cassert>
#include <string_view>

template<typename T>
void test_value(T value, std::string_view str, int base = 10)
{
    char buffer[10];
    __ROSESTD::to_chars(std::begin(buffer), std::end(buffer), value, base);

    assert(str == buffer);
}

template<typename T>
void test()
{
    test_value<T>(17, "17");
    test_value<T>(0, "0");
    if constexpr (__is_signed(T))
    {
        test_value<T>(-29, "-29");
    }

    test_value<T>(0x7b, "7b", 16);
    test_value<T>(0160, "160", 8);

    if constexpr (sizeof(T) * CHAR_BIT > 32)
    {
        test_value<T>(0x1213141516, "1213141516", 16);
    }
}

int main()
{
    test<char>();
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
