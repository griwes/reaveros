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

#include "pmm.h"
#include "../util/log.h"

namespace
{
kernel::pmm::instance global_manager;

static const constexpr auto size_any = 1ull << 63;
static const constexpr auto size_4k = 4 * 1024;
static const constexpr auto size_2M = 2 * 1024 * 1024;
static const constexpr auto size_1G = 1 * 1024 * 1024 * 1024;

constexpr std::string_view memmap_type_to_description(boot_protocol::memory_type type)
{
    switch (type)
    {
        case boot_protocol::memory_type::unusable:
            return "unusable";
        case boot_protocol::memory_type::free:
            return "free";
        case boot_protocol::memory_type::loader:
            return "bootloader";
        case boot_protocol::memory_type::preserve:
            return "preserve for runtime";
        case boot_protocol::memory_type::acpi_reclaimable:
            return "ACPI-reclaimable";
        case boot_protocol::memory_type::persistent:
            return "presistent";

        case boot_protocol::memory_type::kernel:
            return "kernel";
        case boot_protocol::memory_type::initrd:
            return "initrd";
        case boot_protocol::memory_type::paging:
            return "paging structures";
        case boot_protocol::memory_type::memory_map:
            return "memory map";
        case boot_protocol::memory_type::backbuffer:
            return "video backbuffer";
        case boot_protocol::memory_type::log_buffer:
            return "boot log buffer";

        default:
            return "!! INVALID !!";
    }
}
}

namespace kernel::pmm
{
void initialize(std::size_t memmap_size, boot_protocol::memory_map_entry * memmap)
{
    log::println("[PMM] Initializing physical memory manager...");
    log::println(" > Memory map location: {}, size: {}.", memmap, memmap_size);
    log::println(" > Memory map:");

    log::println("| {:-^18} | {:-^16} | {:-^20} |", "", "", "");
    log::println("| {:18} | {:16} | {:20} |", "Physical start", "Length", "Type");
    log::println("| {:-^18} | {:-^16} | {:-^20} |", "", "", "");

    for (auto i = 0ull; i < memmap_size; ++i)
    {
        auto start = phys_addr_t{ memmap[i].physical_start };
        auto size = memmap[i].length;

        while (i + 1 < memmap_size && memmap[i].type == memmap[i + 1].type
               && memmap[i].attributes == memmap[i + 1].attributes
               && start + size == memmap[i + 1].physical_start)
        {
            size += memmap[i + 1].length;
            ++i;
        }

        log::println(
            "| {:#018x} | {:16} | {:20} |", start.value(), size, memmap_type_to_description(memmap[i].type));

        if (memmap[i].type == boot_protocol::memory_type::free)
        {
            auto remaining = size;

#define HANDLE_FRAMES(frame_size, next_size)                                                                 \
    while (start % size_##next_size && remaining >= size_##frame_size)                                       \
    {                                                                                                        \
        global_manager.push_##frame_size(start);                                                             \
        start += size_##frame_size;                                                                          \
        remaining -= size_##frame_size;                                                                      \
    }

            HANDLE_FRAMES(4k, 2M);
            HANDLE_FRAMES(2M, 1G);
            HANDLE_FRAMES(1G, any);
            HANDLE_FRAMES(2M, any);
            HANDLE_FRAMES(4k, any);
        }
    }

    log::println("| {:-^18} | {:-^16} | {:-^20} |", "", "", "");
}

void instance::push_4k(phys_addr_t frame)
{
    if (frame % size_4k)
    {
        PANIC("trying to push a non aligned 4k frame {:x}", frame.value());
    }

    _push(frame, _info_4k);
}

void instance::push_2M(phys_addr_t first_frame)
{
    if (first_frame % size_2M)
    {
        PANIC("trying to push a non aligned 2M frame {x}", first_frame.value());
    }

    _push(first_frame, _info_2M);
}

void instance::push_1G(phys_addr_t first_frame)
{
    if (first_frame % size_1G)
    {
        PANIC("trying to push a non aligned 1G frame {x}", first_frame.value());
    }

    _push(first_frame, _info_1G);
}

void instance::_push(phys_addr_t frame, _stack_info & stack_info)
{
    auto frame_header = phys_ptr_t<_frame_header>{ frame };
    frame_header->next = stack_info.stack;
    stack_info.stack = frame_header;
    ++stack_info.num_frames;
}
}
