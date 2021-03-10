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

#include "console.h"
#include "efi.h"
#include "system_table.h"

namespace efi_loader
{
using EFI_TEXT_RESET = EFIAPI EFI_STATUS (*)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL * self, bool extended_verification);
using EFI_TEXT_STRING = EFIAPI EFI_STATUS (*)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL * self, const char16_t * string);

using EFI_TEXT_TEST_STRING = void (*)();
using EFI_TEXT_QUERY_MODE = void (*)();
using EFI_TEXT_SET_MODE = void (*)();
using EFI_TEXT_SET_ATTRIBUTE = void (*)();

using EFI_TEXT_CLEAR_SCREEN = EFIAPI EFI_STATUS (*)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL * self);

using EFI_TEXT_SET_CURSOR_POSITION = void (*)();
using EFI_TEXT_ENABLE_CURSOR = void (*)();

struct SIMPLE_TEXT_OUTPUT_MODE;

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
{
    EFI_TEXT_RESET reset;
    EFI_TEXT_STRING output_string;
    EFI_TEXT_TEST_STRING test_string;
    EFI_TEXT_QUERY_MODE query_mode;
    EFI_TEXT_SET_MODE set_mode;
    EFI_TEXT_SET_ATTRIBUTE set_attribute;
    EFI_TEXT_CLEAR_SCREEN clear_screen;
    EFI_TEXT_SET_CURSOR_POSITION set_cursor_position;
    EFI_TEXT_ENABLE_CURSOR enable_cursor;
    SIMPLE_TEXT_OUTPUT_MODE * mode;
};

void console::initialize()
{
    system_table->con_out->reset(system_table->con_out, false);
    clear();
}

void console::clear()
{
    system_table->con_out->clear_screen(system_table->con_out);
}

void console::print(const char * str)
{
    print(std::string_view{ str });
}

void console::print(const char16_t * str)
{
    system_table->con_out->output_string(system_table->con_out, str);
}

void console::print(std::string_view str)
{
    char16_t buffer[8192] = { '\0' }; // this is very dumb, but it has to stay for now

    if (str.size() > 8191)
    {
        print(u"[ERR] console::print: tried to print a too long string view.\n\r");
        asm volatile("cli; hlt");
    }

    for (auto i = 0u; i < str.size(); ++i)
    {
        buffer[i] = str[i];
    }

    print(buffer);
}
} // namespace efi_loader
