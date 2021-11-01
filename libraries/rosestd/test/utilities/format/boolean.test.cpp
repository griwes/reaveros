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

void test_value(bool value, std::string_view str, __ROSESTD::__format_string<bool> fmt)
{
    char buffer[100] = "";
    __ROSESTD::format_to(+buffer, fmt, value);

    assert(str == buffer);
}

int main()
{
    test_value(true, "true", "{}");
    test_value(false, "false", "{}");
    test_value(true, "true", "{:s}");
    test_value(false, "false", "{:s}");

    test_value(true, "1", "{:b}");
    test_value(false, "0", "{:b}");

    test_value(true, "1", "{:B}");
    test_value(false, "0", "{:B}");

    test_value(true, "\1", "{:c}");
    test_value(false, "\0", "{:c}");

    test_value(true, "1", "{:d}");
    test_value(false, "0", "{:d}");

    test_value(true, "1", "{:o}");
    test_value(false, "0", "{:o}");

    test_value(true, "1", "{:x}");
    test_value(false, "0", "{:x}");

    test_value(true, "1", "{:X}");
    test_value(false, "0", "{:X}");
}
