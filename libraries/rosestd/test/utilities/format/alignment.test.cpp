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

int main()
{
    test_value("abc", "abc", "{}");
    test_value("abc", "abc   ", "{:6}");
    test_value(1, "     1", "{:6}");
    test_value("abc", "  abc   ", "{:^8}");
    test_value("abc", "***abc***", "{:*^9}");
    test_value("abc", "abc   ", "{:<6}");
    test_value(1, "1     ", "{:<6}");
    test_value("abc", "abc,,,", "{:,<6}");
    test_value("abc", "   abc", "{:>6}");
    test_value(1, "     1", "{:>6}");
    test_value("abc", "~~~abc", "{:~>6}");
}
