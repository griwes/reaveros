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

#include <cstdint>

namespace kernel::amd64::gdt
{
struct [[gnu::packed]] entry
{
    std::uint64_t limit_low : 16;
    std::uint64_t base_low : 24;
    std::uint64_t accessed : 1;
    std::uint64_t read_write : 1;
    std::uint64_t conforming : 1;
    std::uint64_t code : 1;
    std::uint64_t normal : 1;
    std::uint64_t dpl : 2;
    std::uint64_t present : 1;
    std::uint64_t limit_high : 4;
    std::uint64_t available : 1;
    std::uint64_t long_mode : 1;
    std::uint64_t big : 1;
    std::uint64_t granularity : 1;
    std::uint64_t base_high : 8;
};

struct [[gnu::packed]] gdtr_t
{
    std::uint16_t limit;
    entry * address;
};

struct [[gnu::packed]] tss_t
{
    std::uint32_t reserved;
    std::uint64_t rsp0;
    std::uint64_t rsp1;
    std::uint64_t rsp2;
    std::uint64_t reserved2;
    std::uint64_t ist1;
    std::uint64_t ist2;
    std::uint64_t ist3;
    std::uint64_t ist4;
    std::uint64_t ist5;
    std::uint64_t ist6;
    std::uint64_t ist7;
    std::uint64_t reserved3;
    std::uint16_t reserved4;
    std::uint16_t iomap;
};

void initialize(entry (&entries)[7], gdtr_t & gdtr);
void load_gdt(gdtr_t & gdtr);
}
