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

namespace kernel::pmm
{
namespace
{
    instance global_manager;
    std::uintptr_t sub_1M_bottom = 0;
    std::uintptr_t sub_1M_top = 0;

    std::size_t free_frames[arch::vm::page_size_count] = {};
    std::size_t used_frames[arch::vm::page_size_count] = {};

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
            case boot_protocol::memory_type::working_stack:
                return "working stack";

            default:
                return "!! INVALID !!";
        }
    }
}

void instance::push(std::size_t page_layer, phys_addr_t frame)
{
    auto & stack_info = _infos[page_layer];
    std::lock_guard lock(stack_info.lock);

    auto frame_header = phys_ptr_t<_frame_header>{ frame };
    frame_header->next = stack_info.stack;
    stack_info.stack = frame_header;
    ++stack_info.num_frames;
}

phys_addr_t instance::pop(std::size_t page_layer)
{
    auto & stack_info = _infos[page_layer];
    std::lock_guard lock(stack_info.lock);

    if (stack_info.num_frames == 0)
    {
        if (page_layer == arch::vm::page_size_count - 1)
        {
            PANIC("...and now implement cross-pmm-instance rebalancing");
        }

        auto higher_layer_frame = pop(page_layer + 1);
        for (std::size_t i = 0; i < arch::vm::page_sizes[page_layer + 1] / arch::vm::page_sizes[page_layer];
             ++i)
        {
            pmm::push(page_layer, higher_layer_frame + i * arch::vm::page_sizes[page_layer]);
        }
    }

    auto ret = stack_info.stack;
    stack_info.stack = ret->next;
    --stack_info.num_frames;
    return ret.representation();
}

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

        // save first 1MiB
        if (start.value() < 1024 * 1024 && !sub_1M_top)
        {
            sub_1M_bottom = start.value();

            if (size <= 1024 * 1024)
            {
                sub_1M_top = size;
                continue;
            }

            start = phys_addr_t{ 1024 * 1024 };
            size -= 1024 * 1024;

            sub_1M_top = 1024 * 1024;
        }

        if (memmap[i].type == boot_protocol::memory_type::free)
        {
            auto remaining = size;

            auto push = [&](std::size_t i)
            {
                global_manager.push(i, start);
                start += arch::vm::page_sizes[i];
                remaining -= arch::vm::page_sizes[i];
                ++free_frames[i];
            };

            for (std::size_t i = 0; i < arch::vm::page_size_count - 1; ++i)
            {
                while (start % arch::vm::page_sizes[i + 1] && remaining >= arch::vm::page_sizes[i])
                {
                    push(i);
                }
            }

            for (std::size_t i = arch::vm::page_size_count; i > 0; --i)
            {
                while (remaining >= arch::vm::page_sizes[i - 1])
                {
                    push(i - 1);
                }
            }
        }

        else
        {
            switch (memmap[i].type)
            {
                case boot_protocol::memory_type::loader:
                case boot_protocol::memory_type::kernel:
                case boot_protocol::memory_type::initrd:
                case boot_protocol::memory_type::paging:
                case boot_protocol::memory_type::memory_map:
                case boot_protocol::memory_type::backbuffer:
                case boot_protocol::memory_type::log_buffer:
                case boot_protocol::memory_type::working_stack:
                    used_frames[0] += memmap[i].length / arch::vm::page_sizes[0];
                    break;

                default:;
            }
        }
    }

    log::println("| {:-^18} | {:-^16} | {:-^20} |", "", "", "");
}

void report()
{
    std::size_t free = 0;
    std::size_t used = 0;

    for (std::size_t i = 0; i < arch::vm::page_size_count; ++i)
    {
        free += free_frames[i] * arch::vm::page_sizes[i];
        used += used_frames[i] * arch::vm::page_sizes[i];
    }

    auto total = free + used;

    auto free_gib = free / (1024 * 1024 * 1024);
    auto free_mib = (free % (1024 * 1024 * 1024)) / (1024 * 1024);
    auto free_kib = (free % (1024 * 1024)) / 1024;

    auto used_gib = used / (1024 * 1024 * 1024);
    auto used_mib = (used % (1024 * 1024 * 1024)) / (1024 * 1024);
    auto used_kib = (used % (1024 * 1024)) / 1024;

    auto total_gib = total / (1024 * 1024 * 1024);
    auto total_mib = (total % (1024 * 1024 * 1024)) / (1024 * 1024);
    auto total_kib = (total % (1024 * 1024)) / 1024;

    log::println("[PMM] Printing physical memory manager status...");
    log::println(" > Free frames:");
    for (std::size_t i = 0; i < arch::vm::page_size_count; ++i)
    {
        log::println(" >> {}: {}", arch::vm::page_sizes[i], free_frames[i]);
    }
    log::println(" > Used frames:");
    for (std::size_t i = 0; i < arch::vm::page_size_count; ++i)
    {
        log::println(" >> {}: {}", arch::vm::page_sizes[i], used_frames[i]);
    }

    log::println(" > Total free memory: {} GiB {} MiB {} KiB", free_gib, free_mib, free_kib);
    log::println(" > Total used memory: {} GiB {} MiB {} KiB", used_gib, used_mib, used_kib);
    log::println(" > Total memory: {} GiB {} MiB {} KiB", total_gib, total_mib, total_kib);
}

std::uintptr_t get_sub_1M_bottom()
{
    return sub_1M_bottom;
}

std::uintptr_t get_sub_1M_top()
{
    return sub_1M_top;
}

phys_addr_t pop(std::size_t page_layer)
{
    if (page_layer >= arch::vm::page_size_count)
    {
        PANIC("Tried to pop a frame beyond arch-supported frame sizes: {}!", page_layer);
    }

    auto ret = global_manager.pop(page_layer);

    --free_frames[page_layer];
    ++used_frames[page_layer];

    return ret;
}

void push(std::size_t page_layer, phys_addr_t frame)
{
    if (page_layer >= arch::vm::page_size_count)
    {
        PANIC("Tried to push a frame beyond arch-supported frame sizes: {}!", page_layer);
    }

    global_manager.push(page_layer, frame);
    --used_frames[page_layer];
    ++free_frames[page_layer];
}
}
