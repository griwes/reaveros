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

#include "core.h"

#include "../../../scheduler/thread.h"

namespace kernel::amd64::cpu
{
core_local_storage::core_local_storage() = default;

core_local_storage::~core_local_storage() = default;

core_local_storage * get_core_local_storage()
{
    core_local_storage * ret;
    asm volatile("mov %%gs:0, %0" : "=r"(ret));
    return ret;
}

void core::initialize_gdt()
{
    gdt::initialize(_gdt, _gdtr, _tss);
}

void core::load_gdt()
{
    gdt::load_gdt(_gdtr);
}

void core::set_kernel_stack(virt_addr_t stack)
{
    _tss.rsp0 = stack.value();
}

void core::reinit_as(const core & other)
{
    if (_accessed.load(std::memory_order_relaxed))
    {
        PANIC("tried to reinit an already accessed core object!");
    }

    _is_valid = other._is_valid;
    _nmi_valid = other._nmi_valid;

    _id = other._id;
    _apic_id = other._apic_id;
    _acpi_id = other._acpi_id;

    _nmi_vector = other._nmi_vector;
    _nmi_flags = other._nmi_flags;
}
}
