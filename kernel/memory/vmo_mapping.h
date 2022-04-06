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

#include "../util/intrusive_ptr.h"
#include "vmo.h"

#include <user/meta.h>

#include <shared_mutex>

namespace kernel::vm
{
struct address_range
{
    virt_addr_t start;
    virt_addr_t end;
};

class vmo_mapping : public util::intrusive_ptrable<vmo_mapping>
{
public:
    vmo_mapping * tree_parent = nullptr;

    vmo_mapping(vas * as, virt_addr_t start, virt_addr_t end, util::intrusive_ptr<vmo> object, flags fl)
        : _range{ start, end }, _object(std::move(object)), _address_space(as), _mapping_flags(fl)
    {
    }

    const address_range & range() const
    {
        return _range;
    }

    bool has_flags(flags fl) const
    {
        return (std::to_underlying(_mapping_flags) & std::to_underlying(fl)) == std::to_underlying(fl);
    }

    vas * get_vas() const
    {
        return _address_space;
    }

    std::unique_lock<std::shared_mutex> lock() const
    {
        return std::unique_lock{ _lock };
    }

    std::shared_lock<std::shared_mutex> shared_lock() const
    {
        return std::shared_lock{ _lock };
    }

    bool is_invalid()
    {
        return !_valid;
    }

    void release(const std::unique_lock<std::shared_mutex> &)
    {
        _valid = false;
        _object.release(util::drop_count);
        _address_space = nullptr;
        _range = {};
    }

    static rose::syscall::result syscall_rose_mapping_destroy_handler(vmo_mapping * mapping);

private:
    mutable std::shared_mutex _lock;
    address_range _range;
    util::intrusive_ptr<vmo> _object;
    vas * _address_space;
    flags _mapping_flags;
    bool _valid = true;
};

struct vmo_mapping_address_compare
{
    bool operator()(const vmo_mapping & lhs, const vmo_mapping & rhs) const
    {
        return (*this)(lhs.range(), rhs.range());
    }

    bool operator()(const vmo_mapping & lhs, const address_range & rhs) const
    {
        return (*this)(lhs.range(), rhs);
    }

    bool operator()(const address_range & lhs, const vmo_mapping & rhs) const
    {
        return (*this)(lhs, rhs.range());
    }

    bool operator()(const address_range & lhs, const address_range & rhs) const
    {
        return lhs.end.value() <= rhs.start.value();
    }
};
}
