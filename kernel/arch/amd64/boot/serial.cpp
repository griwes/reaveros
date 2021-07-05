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

#include "serial.h"

#include <cstdint>

namespace kernel::boot_serial
{
namespace
{
    void outb(std::uint16_t port, std::uint8_t byte)
    {
        asm volatile("outb %1, %0;" ::"dN"(port), "a"(byte));
    }
}

void put_char(char c)
{
    outb(0x3f8, c);
}
}
