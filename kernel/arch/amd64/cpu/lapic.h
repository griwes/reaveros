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

#include "../../../util/integer_types.h"

namespace kernel::amd64::lapic
{
void initialize(phys_addr_t lapic_base);
std::uint32_t id();

enum class timer_mode
{
    one_shot,
    periodic
};

void write_timer_divisor(std::uint8_t);
void write_timer_counter(std::uint32_t);
std::uint32_t read_timer_counter();
void enable_timer(timer_mode);
}
