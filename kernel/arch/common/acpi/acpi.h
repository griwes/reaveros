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
#include "../../cpu.h"

namespace kernel::acpi
{
void initialize(std::size_t revision, phys_addr_t acpi_root);

struct madt_result
{
    std::size_t core_count;
    phys_addr_t lapic_base;
};
madt_result parse_madt(arch::cpu::core * cores_storage, std::size_t core_count);

struct hpet_result
{
    phys_addr_t base;
    std::uint16_t min_tick;
};
hpet_result parse_hpet();
}
