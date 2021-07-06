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

#include "screen.h"

#include <boot-constants.h>

#include <cstring>
#include "../util/log.h"

asm("bitmap_font:\n"
    ".incbin \"" REAVEROS_KERNEL_SOURCE_ROOT "/boot/IBM_VGA_8x16.bin\"\n");

namespace kernel::boot_screen
{
namespace
{
    bool valid = false;

    boot_protocol::video_mode mode;
    std::uint32_t * framebuffer_base = nullptr;
    std::uint32_t * backbuffer_base = nullptr;
    extern "C" char bitmap_font[];

    std::uint16_t x = 0, y = 0;
    std::uint16_t maxx = 0, maxy = 0;
}

void initialize(
    boot_protocol::video_mode * loader_mode,
    std::size_t memmap_size,
    boot_protocol::memory_map_entry * memmap)
{
    valid = true;

    mode = *loader_mode;
    framebuffer_base = reinterpret_cast<std::uint32_t *>(
        reinterpret_cast<std::uint8_t *>(mode.framebuffer_base) + boot_protocol::physmem_base);

    auto backbuffer_entry =
        boot_protocol::find_entry(memmap_size, memmap, boot_protocol::memory_type::backbuffer);
    backbuffer_base = reinterpret_cast<std::uint32_t *>(
        reinterpret_cast<std::uint8_t *>(backbuffer_entry->physical_start) + boot_protocol::physmem_base);

    maxx = mode.x / 8;
    maxy = mode.y / 16;

    clear();
}

void put_char(char c)
{
    if (!valid)
    {
        return;
    }

    if (c == '\n')
    {
        ++y;
        x = 0;
    }

    else if (c == '\t')
    {
        x += 8 - x % 8;
    }

    else
    {
        auto char_data = &bitmap_font[c * 16];
        auto dest = framebuffer_base + y * mode.ppl * 16 + x * 8;
        auto backdest = backbuffer_base + y * mode.ppl * 16 + x * 8;

        std::uint32_t color = 0;

        switch (mode.format)
        {
            case boot_protocol::pixel_format::rgb:
                [[fallthrough]];
            case boot_protocol::pixel_format::bgr:
                color = 0x00ffffff;
                break;

            case boot_protocol::pixel_format::mask:;
                color = 0xffffffff & (mode.masks.red | mode.masks.green | mode.masks.blue);
                break;
        }

        for (auto row = 0; row < 16; ++row)
        {
            auto row_data = char_data[row];

            for (auto column = 0; column < 8; ++column)
            {
                backdest[column] = (row_data >> (7 - column)) & 1 ? color : 0;
                dest[column] = backdest[column];
            }

            backdest += mode.ppl;
            dest += mode.ppl;
        }

        ++x;
    }

    if (x == maxx)
    {
        x = 0;
        ++y;
    }

    while (y >= maxy)
    {
        scroll();
    }
}

void clear()
{
    if (!valid)
    {
        return;
    }

    std::memset(framebuffer_base, 0, mode.framebuffer_size);
    std::memset(backbuffer_base, 0, mode.framebuffer_size);
    x = 0;
    y = 0;
}

void scroll()
{
    if (!valid)
    {
        return;
    }

    std::memcpy(
        backbuffer_base,
        backbuffer_base + mode.ppl * 16,
        (maxy - 1) * mode.ppl * 16 * sizeof(*backbuffer_base));
    std::memset(backbuffer_base + (maxy - 1) * mode.ppl * 16, 0, mode.ppl * 16 * sizeof(*backbuffer_base));
    std::memcpy(framebuffer_base, backbuffer_base, mode.framebuffer_size);

    --y;
}
}
