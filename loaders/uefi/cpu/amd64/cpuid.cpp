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

#include "../../efi/console.h"
#include "cpuid.h"

#define cpuid(id, a, b, c, d) asm volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(id))

namespace efi_loader::inline amd64
{
cpu_capabilities detect_intel()
{
    cpu_capabilities caps;
    console::print(u" > Intel CPU detected.\n\r");
    caps.manufacturer = cpu_manufacturer::intel;
    return caps;
}

cpu_capabilities detect_amd()
{
    cpu_capabilities caps;
    console::print(u" > AMD CPU detected.\n\r");
    caps.manufacturer = cpu_manufacturer::amd;
    return caps;
}

void detect_brand(cpu_capabilities & caps)
{
    auto buf = reinterpret_cast<std::uint32_t *>(caps.brand_string);

    for (auto i = 0x80000002; i <= 0x80000004; ++i)
    {
        std::uint64_t registers[4];
        cpuid(i, registers[0], registers[1], registers[2], registers[3]);

        for (int j = 0; j < 4; ++j)
        {
            *buf++ = registers[j] & 0xffffffff;
        }
    }
}

cpu_capabilities detect_cpu()
{
    std::uint64_t rbx, _;
    cpuid(0, _, rbx, _, _);

    auto caps = [&]() -> cpu_capabilities {
        switch (rbx)
        {
            case 0x756e6547:
                return detect_intel();

            case 0x68747541:
                return detect_amd();

            default:
                console::print(u" > Unknown CPU manufacturer detected.\n\r");
                return {};
        }
    }();

    std::uint64_t rax;
    cpuid(0x80000000, rax, _, _, _);
    if (rax >= 0x80000004)
    {
        detect_brand(caps);
    }

    console::print(u" > CPU brand string: ", caps.brand_string, u"\n\r");

    return caps;
}
}
