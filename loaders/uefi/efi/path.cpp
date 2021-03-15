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

#include "path.h"
#include "console.h"
#include "efi.h"

namespace efi_loader
{
using EFI_DEVICE_PATH_TO_TEXT_NODE =
    EFIAPI char16_t * (*)(const EFI_DEVICE_PATH_PROTOCOL * device_node, bool display_only, bool allow_shortcuts);

using EFI_DEVICE_PATH_TO_TEXT_PATH =
    EFIAPI char16_t * (*)(const EFI_DEVICE_PATH_PROTOCOL * device_node, bool display_only, bool allow_shortcuts);

constexpr auto EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID =
    EFI_GUID{ 0x8b843e20, 0x8132, 0x4852, { 0x90, 0xcc, 0x55, 0x1a, 0x4e, 0x4a, 0x7f, 0x1c } };

struct EFI_DEVICE_PATH_TO_TEXT_PROTOCOL
{
    EFI_DEVICE_PATH_TO_TEXT_NODE convert_device_node_to_text;
    EFI_DEVICE_PATH_TO_TEXT_PATH convert_device_path_to_text;
};

using EFI_DEVICE_PATH_FROM_TEXT_NODE =
    EFIAPI EFI_DEVICE_PATH_PROTOCOL * (*)(const char16_t * text_device_node);

using EFI_DEVICE_PATH_FROM_TEXT_PATH =
    EFIAPI EFI_DEVICE_PATH_PROTOCOL * (*)(const char16_t * text_device_path);

constexpr auto EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL_GUID =
    EFI_GUID{ 0x5c99a21, 0xc70f, 0x4ad2, { 0x8a, 0x5f, 0x35, 0xdf, 0x33, 0x43, 0xf5, 0x1e } };

struct EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL
{
    EFI_DEVICE_PATH_FROM_TEXT_NODE convert_text_to_device_node;
    EFI_DEVICE_PATH_FROM_TEXT_PATH convert_text_to_device_path;
};

void path::print() const
{
    console::print(_buffer);
}

namespace
{
    EFI_DEVICE_PATH_TO_TEXT_PROTOCOL * to_text_protocol = nullptr;
    EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL * from_text_protocol = nullptr;

    auto to_text_init()
    {
        if (!to_text_protocol)
        {
            to_text_protocol = FIND_PROTOCOL(DEVICE_PATH_TO_TEXT);
            console::print(u" > DEVICE_PATH_TO_TEXT protocol found.\n\r");
        }

        return to_text_protocol;
    }

    auto from_text_init()
    {
        if (!from_text_protocol)
        {
            from_text_protocol = FIND_PROTOCOL(DEVICE_PATH_FROM_TEXT);
            console::print(u" > DEVICE_PATH_FROM_TEXT protocol found.\n\r");
        }

        return from_text_protocol;
    }
}

path::path(EFI_DEVICE_PATH_PROTOCOL * device_path)
    : _buffer{ to_text_init()->convert_device_path_to_text(device_path, false, false) }
{
}

path::path(std::string_view str) : _buffer{ new char16_t[str.size() + 1]{} }
{
    for (std::size_t i = 0; i < str.size(); ++i)
    {
        _buffer[i] = str[i];
    }
    _buffer[str.size()] = u'\0';
}

path::~path()
{
    operator delete[](_buffer);
}

path path::operator/(const char16_t * append) const
{
    char16_t buffer[16 * 1024];
    auto buffer_ptr = buffer;
    for (auto ptr = _buffer; *ptr; ++ptr)
    {
        *buffer_ptr++ = *ptr;
    }
    *buffer_ptr++ = u'\\';
    for (auto ptr = append; *ptr; ++ptr)
    {
        *buffer_ptr++ = *ptr;
    }
    *buffer_ptr = u'\0';

    // this is fishy
    // but I think it's not terribly wrong? IDK
    // at some point I might write a more sane path manipulator thing
    // that allows removing path segments and so on
    // but this works well enough right now
    auto proto = from_text_init()->convert_text_to_device_path(buffer);
    auto ret = path(proto);
    operator delete(proto);
    return ret;
}
}
