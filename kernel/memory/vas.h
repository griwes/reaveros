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

namespace kernel::vm
{
class vas;

std::unique_ptr<vas> create_vas(bool random_map_vdso = false);
std::unique_ptr<vas> adopt_existing_asid(phys_addr_t asid);

class vas : public util::chained_allocatable<vas>
{
    struct _key_t
    {
    };

public:
    friend std::unique_ptr<vas> create_vas(bool);
    friend std::unique_ptr<vas> adopt_existing_asid(phys_addr_t);

    vas(_key_t)
    {
    }

    ~vas();

    phys_addr_t get_asid() const;

    util::intrusive_ptr<vmo_mapping> map_vmo(
        util::intrusive_ptr<vmo> vmo,
        virt_addr_t address,
        flags flags = flags::none);

    std::optional<std::unique_lock<std::mutex>> lock_address_range(
        virt_addr_t start,
        virt_addr_t end,
        bool rw = false);

    template<typename T>
    std::optional<std::unique_lock<std::mutex>> lock_array_mapping(
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
        std::uintptr_t * result_token);

private:
    phys_addr_t _asid;
    std::mutex _lock;

    util::avl_tree<vmo_mapping, vmo_mapping_address_compare, util::intrusive_ptr_preserve_count_traits>
        _mappings;
};
}
