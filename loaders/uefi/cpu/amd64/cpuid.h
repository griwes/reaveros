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

namespace efi_loader::inline amd64
{
enum class cpu_manufacturer
{
    intel,
    amd,
    unknown
};

struct cpu_capabilities
{
    cpu_manufacturer manufacturer = cpu_manufacturer::unknown;
    char brand_string[48] = "Unknown CPU brand";
};

cpu_capabilities detect_cpu();
}
