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

#include "../../../util/integer_types.h"

namespace kernel::amd64::lapic
{
void initialize(phys_addr_t lapic_base);
void ap_initialize();
std::uint32_t id();
void eoi(std::uint8_t);

enum class timer_mode
{
    one_shot,
    periodic
};

void write_timer_divisor(std::uint8_t);
void write_timer_counter(std::uint32_t);
std::uint32_t read_timer_counter();

void enable_timer_irq();
void disable_timer_irq();

enum class ipi_type
{
    init,
    sipi,
    generic,
    nmi
};

enum class broadcast_target
{
    self = 1,
    all,
    others
};

void ipi(std::uint32_t target_apic_id, ipi_type type, std::uint8_t data);
void broadcast(broadcast_target target, ipi_type type, std::uint8_t data);
}
