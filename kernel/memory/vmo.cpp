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

#include "vmo.h"

#include <iterator>

namespace kernel::vm
{
util::intrusive_ptr<vmo> create_physical_vmo(
    phys_addr_t base,
    std::size_t length,
    std::uint_least8_t aligned_to_page_level)
{
    length +=
        (arch::vm::page_sizes[aligned_to_page_level] - length) % arch::vm::page_sizes[aligned_to_page_level];

    auto ret = util::make_intrusive<vmo>(vmo::_key_t{});

    ret->_type = vmo_type::physical;
    ret->_length = length;
    ret->_aligned_to_page_level = aligned_to_page_level;

    ret->_state.physical = { .base = base };

    return ret;
}

util::intrusive_ptr<vmo> create_sparse_vmo(std::size_t length, std::uint_least8_t aligned_to_page_level)
{
    length +=
        (arch::vm::page_sizes[aligned_to_page_level] - length) % arch::vm::page_sizes[aligned_to_page_level];

    auto ret = util::make_intrusive<vmo>(vmo::_key_t{});

    ret->_type = vmo_type::sparse;
    ret->_length = length;
    ret->_aligned_to_page_level = aligned_to_page_level;

    auto element = std::make_unique<vmo::_sparse_vmo_element>();
    element->offset = 0;

    ret->_state.sparse = {};
    ret->_state.sparse.elements.insert(std::move(element));

    return ret;
}

vmo::~vmo()
{
    switch (_type)
    {
        case vmo_type::physical:
            _state.physical.~_physical_vmo_state();
            break;

        case vmo_type::sparse:
            for (auto && element : _state.sparse.elements)
            {
                if (element.backing_address)
                {
                    pmm::push(_aligned_to_page_level, *element.backing_address);
                }
            }

            _state.sparse.~_sparse_vmo_state();
            break;
    }
}

void vmo::commit_between_offsets(std::size_t start_offset, std::size_t end_offset)
{
    switch (_type)
    {
        case vmo_type::physical:
            return;

        case vmo_type::sparse:
        {
            auto & elements = _state.sparse.elements;

            auto begin = elements.lower_bound(start_offset);
            if (begin->offset > start_offset)
            {
                --begin;
            }

            auto end = elements.upper_bound(end_offset);

            while (begin != end)
            {
                if (begin->backing_address)
                {
                    ++begin;
                    continue;
                }

                auto next = begin;
                ++next;
                const auto next_offset = next == elements.end() ? _length : next->offset;

                const auto element_length = arch::vm::page_sizes[_aligned_to_page_level];

                while (begin->offset + element_length < next_offset)
                {
                    begin->backing_address = pmm::pop(_aligned_to_page_level);

                    auto new_element = std::make_unique<_sparse_vmo_element>();
                    new_element->offset = begin->offset + element_length;
                    begin = elements.insert(next, std::move(new_element));
                }

                begin->backing_address = pmm::pop(_aligned_to_page_level);
                ++begin;
            }

            return;
        }

        default:
            PANIC("commit_between_offsets() called on an VMO of unsupported type");
    }
}

void vmo::commit_all()
{
    commit_between_offsets(0, _length);
}
}
