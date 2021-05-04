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

namespace kernel::amd64::irq
{
static const constexpr auto free_start = 0x20;
static const constexpr auto free_end = 0xf0;

static const constexpr auto lapic_timer = 0xf0;
static const constexpr auto lapic_spurious = 0xff;

struct [[gnu::packed]] context
{
    std::uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    std::uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    std::uint64_t number, error;
    std::uint64_t rip, cs, rflags, rsp, ss;
};

void handle(context & ctx);
}
