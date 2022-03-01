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

#include "../arch/int.h"

#include <cstdint>

namespace kernel::util
{
class [[nodiscard]] interrupt_guard
{
public:
    interrupt_guard() : _enable(arch::cpu::disable_interrupts())
    {
    }

    ~interrupt_guard()
    {
        if (_enable)
        {
            arch::cpu::enable_interrupts();
        }
    }

    interrupt_guard(const interrupt_guard &) = delete;
    interrupt_guard(interrupt_guard &&) = delete;
    interrupt_guard & operator=(const interrupt_guard &) = delete;
    interrupt_guard & operator=(interrupt_guard &&) = delete;

private:
    bool _enable = false;
};
}
