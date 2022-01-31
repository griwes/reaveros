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

#include "helpers.h"

namespace kernel::util
{
template<typename T, typename PointerTraits = unique_ptr_traits<T>>
class fifo
{
public:
    void push_back(typename PointerTraits::pointer element)
    {
        if (!_head)
        {
            _head = PointerTraits::unwrap(std::move(element));
            _tail = _head;

            _head->prev = nullptr;
            _head->next = nullptr;

            return;
        }

        _tail->next = PointerTraits::unwrap(std::move(element));
        _tail->next->next = nullptr;
        _tail->next->prev = _tail;
        _tail = _tail->next;
    }

    bool empty() const
    {
        return _head == nullptr;
    }

private:
    T * _head = nullptr;
    T * _tail = nullptr;
};
};
