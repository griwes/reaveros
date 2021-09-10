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

#include "../arch/vm.h"
#include "../util/pointer_types.h"

#include <boot-memmap.h>

#include <mutex>

namespace kernel::pmm
{
class instance
{
public:
    instance() = default;

    void push(std::size_t page_layer, phys_addr_t frame);
    phys_addr_t pop(std::size_t page_layer);

private:
    struct _frame_header
    {
        phys_ptr_t<_frame_header> next;
    };

    struct _stack_info
    {
        std::mutex lock;
        phys_ptr_t<_frame_header> stack{ nullptr };
        std::size_t num_frames = 0;
    };

    _stack_info _infos[arch::vm::page_size_count];
};

void initialize(std::size_t memmap_size, boot_protocol::memory_map_entry * memmap);
void report();

std::uintptr_t get_sub_1M_bottom();
std::uintptr_t get_sub_1M_top();

phys_addr_t pop(std::size_t page_layer);
void push(std::size_t page_layer, phys_addr_t);
}
