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

#include <cstddef>
#include <cstdint>

namespace boot_protocol
{
enum class memory_type : std::uint32_t
{
    unusable,
    free,
    loader,
    preserve,
    acpi_reclaimable,
    persistent,

    kernel,     // must be present exactly once
    initrd,     // must be present exactly once
    paging,     // may be present an arbitrary number of times
    memory_map, // must be present exactly once
    backbuffer, // must be present exactly once if video mode information is passed to the kernel
    log_buffer  // must be present exactly once and be exactly 2MiB in size
};

struct memory_map_entry
{
    std::uintptr_t physical_start;
    std::size_t length;
    memory_type type;
    std::uint32_t attributes;
};

inline memory_map_entry * find_entry(
    std::size_t memmap_size,
    memory_map_entry * memmap,
    boot_protocol::memory_type type)
{
    for (auto i = 0ull; i < memmap_size; ++i)
    {
        if (memmap[i].type == type)
        {
            return memmap + i;
        }
    }

    return nullptr;
}
}
