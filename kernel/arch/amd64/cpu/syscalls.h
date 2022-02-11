/*
 * Copyright © 2022 Michał 'Griwes' Dominiak
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

#include "thread.h"

#include <cstdint>

namespace kernel::amd64::syscalls
{
struct [[gnu::packed]] context
{
    std::uint64_t r15, r14, r13, r12, rflags, r10, r9, r8;
    std::uint64_t rbp, rdi, rsi, user_rsp, user_rip, rbx, rax;

    void save_to(thread::context *) const;
    void load_from(const thread::context *);
};

void initialize();
}
