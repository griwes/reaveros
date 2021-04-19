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

asm("bitmap_font:\n"
    ".incbin \"" REAVEROS_KERNEL_SOURCE_ROOT "/boot/IBM_VGA_8x16.bin\"\n");

namespace
{
boot_protocol::video_mode mode;
std::uint8_t * framebuffer_base = nullptr;
std::uint8_t * backbuffer_base = nullptr;
extern "C" char bitmap_font[];
}

namespace kernel::boot_screen
{
void initialize(
    boot_protocol::video_mode * loader_mode,
    std::size_t memmap_size,
    boot_protocol::memory_map_entry * memmap)
{
    mode = *loader_mode;
    framebuffer_base = reinterpret_cast<std::uint8_t *>(mode.framebuffer_base) + boot_protocol::physmem_base;

    auto backbuffer_entry =
        boot_protocol::find_entry(memmap_size, memmap, boot_protocol::memory_type::backbuffer);
    backbuffer_base =
        reinterpret_cast<std::uint8_t *>(backbuffer_entry->physical_start) + boot_protocol::physmem_base;
}
}
