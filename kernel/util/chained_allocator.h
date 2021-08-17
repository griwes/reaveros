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

#include "../memory/pmm.h"
#include "log.h"

namespace kernel::util
{
namespace
{
    template<typename T>
    T * chained_head = nullptr;
}

template<typename T>
struct chained_allocatable
{
    T * prev = nullptr;
    T * next = nullptr;

    static void * operator new(std::size_t size);
    static void operator delete(void * p, std::size_t size);
};

template<typename T>
T * allocate_chained()
{
    static_assert(sizeof(T) <= arch::vm::page_sizes[0], "chained_allocatable must fit in a frame");

    if (!chained_head<T>)
    {
        auto frame_address = pmm::pop(0);
        T * current = static_cast<phys_ptr_t<T>>(frame_address).value();
        T * next = nullptr;
        T * last = current;

        for (std::size_t i = 0; (i + 1) * sizeof(T) <= 4096; ++i)
        {
            if (current == last)
            {
                current->next = next;
                current->prev = nullptr;
                next = current;
                ++current;
            }
        }

        // TODO: lock
        last->next = chained_head<T>;
        if (chained_head<T>)
        {
            chained_head<T>->prev = last;
        }
        chained_head<T> = next;
    }

    auto ret = chained_head<T>;
    chained_head<T> = chained_head<T>->next;
    return ret;
}

template<typename T>
void deallocate_chained(T * ptr)
{
    // TODO: lock
    ptr->prev = nullptr;
    ptr->next = chained_head<T>;
    if (chained_head<T>)
    {
        chained_head<T>->prev = ptr;
    }
    chained_head<T> = ptr;
}

template<typename T>
void * chained_allocatable<T>::operator new(std::size_t size)
{
    {
        if (size != sizeof(T))
        {
            PANIC("Tried to allocate more than one chained object.");
        }

        return allocate_chained<T>();
    }
}

template<typename T>
void chained_allocatable<T>::operator delete(void * p, std::size_t)
{
    deallocate_chained(static_cast<T *>(p));
}
}
