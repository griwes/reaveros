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

#include "gdt.h"

#include <cstring>

namespace
{
void setup_gdte(kernel::amd64::gdt::entry & entry, bool code, bool user)
{
    entry.normal = 1;

    entry.code = code;
    entry.dpl = user * 3;
    entry.long_mode = 1;
    entry.present = 1;
    entry.read_write = 1;
}
}

extern "C" void load_gdtr(void * gdtr);

namespace kernel::amd64::gdt
{
void initialize(entry (&entries)[7], gdtr_t & gdtr)
{
    std::memset(entries, 0, sizeof(entries));

    gdtr.address = entries;
    gdtr.limit = sizeof(entries) - 1;

    setup_gdte(entries[1], true, false);  // 0x8 kernel code
    setup_gdte(entries[2], false, false); // 0x10 kernel data
    setup_gdte(entries[3], true, true);   // 0x18 userspace code
    setup_gdte(entries[4], false, true);  // 0x20 userspace data
}

void load_gdt(gdtr_t & gdtr)
{
    load_gdtr(&gdtr);
}
}
