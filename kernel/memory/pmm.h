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

#include <boot-memmap.h>

#include "../util/pointer_types.h"

namespace kernel::pmm
{
class instance
{
public:
    instance() = default;

    void push_4k(phys_addr_t frame);
    void push_2M(phys_addr_t first_frame);
    void push_1G(phys_addr_t first_frame);

private:
    struct _frame_header
    {
        phys_ptr_t<_frame_header> next;
    };

    struct _stack_info
    {
        phys_ptr_t<_frame_header> stack{ nullptr };
        std::size_t num_frames = 0;
    };

    void _push(phys_addr_t frame, _stack_info & list);

    _stack_info _info_4k;
    _stack_info _info_2M;
    _stack_info _info_1G;
};

void initialize(std::size_t memmap_size, boot_protocol::memory_map_entry * memmap);
void report();
}
