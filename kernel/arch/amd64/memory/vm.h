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

namespace kernel::amd64::vm
{
constexpr std::size_t page_size_count = 3;
constexpr std::size_t page_sizes[] = { 4 * 1024, 2 * 1024 * 1024, 1 * 1024 * 1024 * 1024 };

void map_physical(virt_addr_t begin, virt_addr_t end, phys_addr_t physical);

void unmap(virt_addr_t begin, virt_addr_t end, bool free_physical = true);

void unmap_lower_half();
}
