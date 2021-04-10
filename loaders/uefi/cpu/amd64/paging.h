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

#include <cstddef>
#include <cstdint>

namespace efi_loader::inline amd64
{
void prepare_paging();
void vm_map(void * phys, std::size_t size, std::uintptr_t virt);
void vm_map_large(void * phys, std::size_t size, std::uintptr_t virt);
void vm_map_huge(void * phys, std::size_t size, std::uintptr_t virt);

inline constexpr auto kernel_base = 0xffffffff80000000;
inline constexpr auto physmem_base = 0xffff800000000000;
}
