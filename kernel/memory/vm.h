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

#pragma once

#include "../util/pointer_types.h"

namespace kernel::util
{
template<typename T>
class intrusive_ptr;
}

namespace kernel::vm
{
class vmo;

virt_addr_t allocate_address_range(std::size_t size);
void set_vdso_vmo(util::intrusive_ptr<vmo> vdso);

enum class flags : std::uintptr_t
{
    none = 0,
    user = 1 << 0,
    read_only = 1 << 1
};

inline bool operator&(flags lhs, flags rhs)
{
    return static_cast<std::uintptr_t>(lhs) & static_cast<std::uintptr_t>(rhs);
}

inline flags operator|(flags lhs, flags rhs)
{
    return static_cast<flags>(static_cast<std::uintptr_t>(lhs) | static_cast<std::uintptr_t>(rhs));
}
}
