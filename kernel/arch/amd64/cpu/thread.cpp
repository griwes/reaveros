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

#include "thread.h"

namespace kernel::amd64::thread
{
void context::set_userspace()
{
    cs = 0x1b;
    ss = 0x23;
}

void context::set_instruction_pointer(virt_addr_t address)
{
    rip = address.value();
}

void context::set_stack_pointer(virt_addr_t address)
{
    rsp = address.value();
}

void context::set_argument(std::size_t idx, std::uintptr_t value)
{
    switch (idx)
    {
        case 0:
            rdi = value;
            break;

        case 1:
            rsi = value;
            break;

        case 2:
            rdx = value;
            break;

        case 3:
            rcx = value;
            break;

        case 4:
            r8 = value;
            break;

        default:
            PANIC("Unsupported thread context argument index: #{} = {}!", idx, value);
    }
}
}
