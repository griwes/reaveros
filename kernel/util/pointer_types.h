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

#include "address_types.h"

namespace kernel
{
template<typename T>
class phys_ptr_t
{
public:
    phys_ptr_t() = default;

    explicit phys_ptr_t(tagged_address_type<std::uintptr_t, physical_address_tag> address) : _repr(address)
    {
    }

    explicit phys_ptr_t(T * pointer)
        : _repr(reinterpret_cast<std::uintptr_t>(pointer) - boot_protocol::physmem_base)
    {
    }

    T * value() const
    {
        return reinterpret_cast<T *>(_repr.value() + boot_protocol::physmem_base);
    }

    T * operator->() const
    {
        return value();
    }

    decltype(auto) operator*() const requires(!std::same_as<T, void>)
    {
        return *value();
    }

private : tagged_address_type<std::uintptr_t, physical_address_tag> _repr;
};

static_assert(sizeof(phys_ptr_t<void>) == sizeof(void *));
}
