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

#include "gdt.h"
#include "../../../memory/pmm.h"
#include "../../../memory/vm.h"
#include "../memory/vm.h"

#include <cstring>

namespace kernel::amd64::gdt
{
namespace
{
    void setup_gdte(entry & entry, bool code, bool user)
    {
        entry.normal = 1;

        entry.code = code;
        entry.dpl = user * 3;
        entry.long_mode = 1;
        entry.present = 1;
        entry.read_write = 1;
    }

    void setup_tss(entry * tss_entry, tss_t & tss)
    {
        tss.iomap = sizeof(tss_t);

        tss_entry->base_low = reinterpret_cast<std::uint64_t>(&tss) & 0xffffff;
        tss_entry->base_high = (reinterpret_cast<std::uint64_t>(&tss) >> 24) & 0xff;
        *reinterpret_cast<std::uint32_t *>(tss_entry + 1) =
            (reinterpret_cast<std::uint64_t>(&tss) >> 32) & 0xffffffff;

        tss_entry->limit_low = (sizeof(tss_t) & 0xffff) - 1;
        tss_entry->limit_high = sizeof(tss_t) >> 16;

        tss_entry->accessed = 1;
        tss_entry->code = 1;
        tss_entry->present = 1;
        tss_entry->dpl = 3;

        const auto page_size = vm::page_sizes[0];

        virt_addr_t stack_bases[3];

        for (auto i = 0; i < 3; ++i)
        {
            stack_bases[i] = kernel::vm::allocate_address_range(32 * page_size);

            for (auto j = 1; j < 32; ++j)
            {
                vm::map_physical(
                    stack_bases[i] + j * page_size, stack_bases[i] + (j + 1) * page_size, pmm::pop(0));
            }
        }

        tss.ist1 = (stack_bases[0] + 32 * page_size).value();
        tss.ist2 = (stack_bases[1] + 32 * page_size).value();
        tss.ist3 = (stack_bases[2] + 32 * page_size).value();
    }
}

extern "C" void load_gdtr(void * gdtr);

void initialize(entry (&entries)[7], gdtr_t & gdtr, tss_t & tss)
{
    std::memset(entries, 0, sizeof(entries));

    gdtr.address = entries;
    gdtr.limit = sizeof(entries) - 1;

    setup_gdte(entries[1], true, false);  // 0x8 kernel code
    setup_gdte(entries[2], false, false); // 0x10 kernel data
    setup_gdte(entries[3], true, true);   // 0x18 userspace code
    setup_gdte(entries[4], false, true);  // 0x20 userspace data
    setup_tss(entries + 5, tss);          // 0x28 tss
}

void load_gdt(gdtr_t & gdtr)
{
    load_gdtr(&gdtr);
}
}
