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

#include "../util/pointer_types.h"

namespace bootinit::addresses
{
constexpr kernel::virt_addr_t ip(0x10000);
constexpr kernel::virt_addr_t vdso(0xa0000);
constexpr kernel::virt_addr_t initrd(0x100000);
constexpr kernel::virt_addr_t top_of_stack(0x80000000);
}
