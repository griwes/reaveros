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

#include "vas.h"

namespace kernel::vm
{
std::unique_ptr<vas> create_vas(bool randomly_map_vdso)
{
    auto ret = std::make_unique<vas>(vas::_key_t{});

    ret->_asid = arch::vm::clone_upper_half();

    if (randomly_map_vdso)
    {
        PANIC("Implement random mapping of vdso!");
    }

    return ret;
}

std::unique_ptr<vas> adopt_existing_asid(phys_addr_t asid)
{
    auto ret = std::make_unique<vas>(vas::_key_t{});

    ret->_asid = asid;

    return ret;
}

vas::~vas()
{
    PANIC("this requires an amount of work tbh");
}

phys_addr_t vas::get_asid() const
{
    return _asid;
}

void vas::map_vmo(util::intrusive_ptr<vmo> vm_object, virt_addr_t mapping_base, flags fl)
{
    auto page_size = arch::vm::page_sizes[vm_object->page_alignment_level()];
    auto alignment_mask = page_size - 1;

    if (mapping_base.value() & alignment_mask)
    {
        PANIC(
            "tried to map a VMO aligned to page level {} (page size: {}) to an address not aligned to the "
            "page alignment ({:#018x})!",
            vm_object->page_alignment_level(),
            page_size,
            mapping_base.value());
    }

    log::println("mapping for {:#018x}", _asid.value());

    switch (vm_object->type())
    {
        case vmo_type::physical:
            arch::vm::map_physical(
                this, mapping_base, mapping_base + vm_object->length(), vm_object->base(), fl);
            break;

        case vmo_type::sparse:
            for (auto && element : vm_object->sparse_elements())
            {
                if (!element.backing_address)
                {
                    PANIC("mapping uncommitted sparse VMOs is not supported yet (TODO after on-demand "
                          "mapping)");
                }

                arch::vm::map_physical(
                    this,
                    mapping_base + element.offset,
                    mapping_base + element.offset + page_size,
                    *element.backing_address,
                    fl);
            }
            break;

        default:
            PANIC("unknown vmo type!");
    }
}
}
