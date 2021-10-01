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

#include "../util/avl_tree.h"
#include "../util/handle.h"
#include "../util/intrusive_ptr.h"

#include <optional>

namespace kernel::scheduler
{
class process;
}

namespace kernel::vm
{
enum class vmo_type
{
    physical,
    sparse
};

class vmo : public util::intrusive_ptrable<vmo>
{
    struct _key_t
    {
    };

public:
    friend util::intrusive_ptr<vmo> create_physical_vmo(phys_addr_t, std::size_t, std::uint_least8_t);
    friend util::intrusive_ptr<vmo> create_sparse_vmo(std::size_t, std::uint_least8_t);

    vmo(_key_t)
    {
    }

    ~vmo();

    vmo_type type() const
    {
        return _type;
    }

    std::size_t length() const
    {
        return _length;
    }

    std::uint_least8_t page_alignment_level() const
    {
        return _aligned_to_page_level;
    }

    phys_addr_t base() const
    {
        if (_type != vmo_type::physical)
        {
            PANIC("tried to get a base address of a non-physical VMO!");
        }

        return _state.physical.base;
    }

    auto & sparse_elements() const
    {
        if (_type != vmo_type::sparse)
        {
            PANIC("tied to get sparse elements of a non-sparse VMO!");
        }

        return _state.sparse.elements;
    }

    void commit_between_offsets(std::size_t start, std::size_t end);
    void commit_all();

private:
    vmo_type _type;
    std::size_t _length;
    std::uint_least8_t _aligned_to_page_level;

    struct _physical_vmo_state
    {
        phys_addr_t base;
    };

    struct _sparse_vmo_element : util::treeable<_sparse_vmo_element>
    {
        std::size_t offset;
        std::optional<phys_addr_t> backing_address;
    };

    struct _sparse_vmo_element_comparator
    {
        bool operator()(const _sparse_vmo_element & lhs, const _sparse_vmo_element & rhs) const
        {
            return lhs.offset < rhs.offset;
        }

        bool operator()(const _sparse_vmo_element & lhs, std::size_t value) const
        {
            return lhs.offset < value;
        }

        bool operator()(std::size_t value, const _sparse_vmo_element & rhs) const
        {
            return value < rhs.offset;
        }
    };

    struct _sparse_vmo_state
    {
        util::avl_tree<_sparse_vmo_element, _sparse_vmo_element_comparator> elements;
    };

    union _vmo_state
    {
        _vmo_state()
        {
        }

        ~_vmo_state()
        {
        }

        _physical_vmo_state physical;
        _sparse_vmo_state sparse;
    };

    _vmo_state _state;
};

util::intrusive_ptr<vmo> create_physical_vmo(
    phys_addr_t base,
    std::size_t length,
    std::uint_least8_t aligned_to_page_level = 0);
util::intrusive_ptr<vmo> create_sparse_vmo(std::size_t length, std::uint_least8_t aligned_to_page_level = 0);
}
