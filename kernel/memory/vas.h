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

#include "vmo.h"

#include "../util/avl_tree.h"
#include "../util/chained_allocator.h"
#include "vmo_mapping.h"

#include <user/meta.h>

#include <memory>
#include <shared_mutex>

namespace kernel::vm
{
class vas;

util::intrusive_ptr<vas> create_vas(bool random_map_vdso = false);
util::intrusive_ptr<vas> adopt_existing_asid(phys_addr_t asid);

class vas : public util::intrusive_ptrable<vas>
{
    struct _key_t
    {
    };

public:
    friend util::intrusive_ptr<vas> create_vas(bool);
    friend util::intrusive_ptr<vas> adopt_existing_asid(phys_addr_t);

    vas(_key_t)
    {
    }

    ~vas();

    phys_addr_t get_asid() const;
    bool claim_for_process();

    std::optional<virt_addr_t> get_vdso_base() const
    {
        if (!_vdso_mapping)
        {
            return std::nullopt;
        }
        return _vdso_mapping->range().start;
    }

    util::intrusive_ptr<vmo_mapping> map_vmo(
        util::intrusive_ptr<vmo> vmo,
        virt_addr_t address,
        flags flags = flags::none);

    void unmap(vmo_mapping * mapping);

    std::optional<std::shared_lock<std::shared_mutex>> lock_address_range(
        virt_addr_t start,
        virt_addr_t end,
        bool rw = false);

    template<typename T>
    std::optional<std::shared_lock<std::shared_mutex>> lock_array_mapping(
        T * ptr,
        std::size_t count,
        bool rw = false)
    {
        T * end = ptr + count;
        return lock_address_range(
            virt_addr_t(reinterpret_cast<std::uintptr_t>(ptr)),
            virt_addr_t(reinterpret_cast<std::uintptr_t>(end)),
            rw);
    }

    static rose::syscall::result syscall_rose_vas_create_handler(
        kernel_caps_t *,
        std::uintptr_t * result_token,
        rose::syscall::vdso_mapping_info * vdso_info);
    static rose::syscall::result syscall_rose_mapping_create_handler(
        vas * vas_token,
        vmo * vmo_token,
        std::uintptr_t address,
        std::uintptr_t flags,
        std::uintptr_t * token);

private:
    phys_addr_t _asid;
    std::mutex _lock;
    bool _was_claimed_for_process = false;

    util::avl_tree<vmo_mapping, vmo_mapping_address_compare, util::intrusive_ptr_preserve_count_traits>
        _mappings;
    util::intrusive_ptr<vmo_mapping> _vdso_mapping;
};
}
