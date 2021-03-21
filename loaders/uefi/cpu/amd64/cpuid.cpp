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

#include <cstdint>
#include <cstring>

#include "../../efi/console.h"
#include "cpuid.h"

#define cpuid(id, a, b, c, d) asm volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(id))

namespace efi_loader::inline amd64
{
void detect_intel(cpu_capabilities & caps)
{
    console::print(u" > CPU vendor: Intel\n\r");
    caps.manufacturer = cpu_manufacturer::intel;
}

void detect_amd(cpu_capabilities & caps)
{
    console::print(u" > CPU vendor: AMD\n\r");
    caps.manufacturer = cpu_manufacturer::amd;
}

void detect_brand(cpu_capabilities & caps)
{
    auto buf = caps.brand_string;

    for (auto i = 0x80000002; i <= 0x80000004; ++i)
    {
        std::uint32_t registers[4];
        cpuid(i, registers[0], registers[1], registers[2], registers[3]);

        std::memcpy(buf, registers, sizeof(registers));
        buf += sizeof(registers);
    }
}

cpu_capabilities detect_cpu()
{
    cpu_capabilities caps;
    std::uint32_t ebdcx[3], _;
    cpuid(0, _, ebdcx[0], ebdcx[2], ebdcx[1]);

    std::string_view vendor{ reinterpret_cast<char *>(ebdcx), sizeof(ebdcx) };

    if (vendor == std::string_view{ "GenuineIntel" })
    {
        detect_intel(caps);
    }
    else if (vendor == std::string_view{ "AuthenticAMD" })
    {
        detect_amd(caps);
    }
    else
    {
        console::print(u"> CPU vendor: unknown (", vendor, u")\r\n");
    }

    std::uint32_t eax;
    cpuid(0x80000000, eax, _, _, _);
    if (eax >= 0x80000004)
    {
        detect_brand(caps);
    }

    console::print(u" > CPU brand string: ", caps.brand_string, u"\n\r");

    return caps;
}
}
