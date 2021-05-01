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

#include <concepts>
#include <cstdint>

#include <boot-constants.h>

namespace kernel
{
struct physical_address_tag;
struct virtual_address_tag;

template<typename T, typename Tag>
class tagged_address_type
{
public:
    tagged_address_type() = default;

    explicit tagged_address_type(std::uintptr_t repr) : _repr(repr)
    {
    }

    T value() const
    {
        return _repr;
    }

#define DEFINE_OPERATOR(op)                                                                                  \
    template<typename Other>                                                                                 \
    tagged_address_type operator op(Other oth) requires std::integral<Other>                                 \
    {                                                                                                        \
        return tagged_address_type{ _repr op oth };                                                          \
    }

    DEFINE_OPERATOR(+);
    DEFINE_OPERATOR(-);

#undef DEFINE_OPERATOR

#define DEFINE_OPERATOR(op)                                                                                  \
    template<typename Other>                                                                                 \
    Other operator op(Other oth) requires std::integral<Other>                                               \
    {                                                                                                        \
        return _repr op oth;                                                                                 \
    }

    DEFINE_OPERATOR(%);

#undef DEFINE_OPERATOR

#define DEFINE_OPERATOR(op)                                                                                  \
    template<typename Other>                                                                                 \
    tagged_address_type & operator op(Other oth) requires std::integral<Other>                               \
    {                                                                                                        \
        _repr op oth;                                                                                        \
        return *this;                                                                                        \
    }

    DEFINE_OPERATOR(+=);
    DEFINE_OPERATOR(-=);

#undef DEFINE_OPERATOR

#define DEFINE_OPERATOR(op)                                                                                  \
    template<typename Other>                                                                                 \
    auto operator op(Other oth) requires std::integral<Other>                                                \
    {                                                                                                        \
        return _repr op oth;                                                                                 \
    }

    DEFINE_OPERATOR(==);
    DEFINE_OPERATOR(<=>);

#undef DEFINE_OPERATOR

private:
    std::uintptr_t _repr;
};

using phys_addr_t = tagged_address_type<std::uintptr_t, physical_address_tag>;
using virt_addr_t = tagged_address_type<std::uintptr_t, virtual_address_tag>;

static_assert(sizeof(phys_addr_t) == sizeof(std::uintptr_t));
static_assert(sizeof(virt_addr_t) == sizeof(std::uintptr_t));
}
