/*
 * Copyright © 2021-2022 Michał 'Griwes' Dominiak
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

#include "../../../util/chained_allocator.h"

namespace kernel::amd64::thread
{
struct context
{
    std::uint64_t rax = 0, rbx = 0, rcx = 0, rdx = 0;
    std::uint64_t rsi = 0, rdi = 0, rsp = 0, rbp = 0;
    std::uint64_t r8 = 0, r9 = 0, r10 = 0, r11 = 0;
    std::uint64_t r12 = 0, r13 = 0, r14 = 0, r15 = 0;
    std::uint64_t cs = 0x8, ss = 0x10;
    std::uint64_t rip = 0, rflags = 1 << 9;

    bool can_sysret = false;

    void set_userspace();
    void set_instruction_pointer(virt_addr_t address);
    void set_stack_pointer(virt_addr_t address);
    void set_argument(std::size_t idx, std::size_t value);

    template<typename Int, typename Tag>
    requires(std::is_unsigned_v<Int> && sizeof(Int) <= sizeof(std::uint64_t)) void set_argument(
        std::size_t idx,
        tagged_integer_type<Int, Tag> value)
    {
        set_argument(idx, value.value());
    }
};
}
