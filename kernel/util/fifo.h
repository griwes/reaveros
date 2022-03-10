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
template<typename T, template<typename> typename UnboundTraits = unique_ptr_traits>
class fifo
{
    using Traits = UnboundTraits<T>;

public:
    void push_back(typename Traits::pointer element)
    {
        if (!_head)
        {
            _head = Traits::unwrap(std::move(element));
            _tail = _head;

            _head->next = nullptr;

            ++_size;

            return;
        }

        _tail->next = Traits::unwrap(std::move(element));
        _tail->next->next = nullptr;
        _tail = _tail->next;

        ++_size;
    }

    typename Traits::pointer pop_front()
    {
        if (empty())
        {
            return {};
        }

        --_size;

        auto ret_raw = _head;
        _head = _head->next;
        return Traits::create(ret_raw);
    }

    bool empty() const
    {
        return _head == nullptr;
    }

    std::size_t size() const
    {
        return _size;
    }

private:
    T * _head = nullptr;
    T * _tail = nullptr;

    std::size_t _size = 0;
};
};
