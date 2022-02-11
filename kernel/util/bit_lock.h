/*
 * Copyright © 2022 Michał 'Griwes' Dominiak
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

#include <cstdint>

// TODO: rewrite as atomic_ref?

namespace kernel::util
{
template<std::uint_least8_t Bit>
class bit_lock
{
public:
    template<typename T>
    bit_lock(T * address) : _address{ reinterpret_cast<std::uint64_t *>(address) }
    {
        static_assert(sizeof(T) * CHAR_BIT > Bit);

        while (__atomic_fetch_or(_address, static_cast<std::uint64_t>(1) << Bit, __ATOMIC_ACQ_REL)
               & (static_cast<std::uint64_t>(1) << Bit))
        {
            asm volatile("pause" ::: "memory");
        }
    }

    ~bit_lock()
    {
        if (_address)
        {
            *_address &= ~(static_cast<std::uint64_t>(1) << Bit);
        }
    }

    bit_lock(const bit_lock &) = delete;

    bit_lock(bit_lock && rhs) : _address{ rhs._address }
    {
        rhs._address = nullptr;
    }

    bit_lock & operator=(const bit_lock &) = delete;
    bit_lock & operator=(bit_lock &&) = delete;

private:
    std::uint64_t * _address;
};
}
