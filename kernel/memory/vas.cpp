/*
 * Copyright © 2021-2025 Michał 'Griwes' Dominiak
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

#include "vas.h"
#include "../arch/cpu.h"
#include "../scheduler/thread.h"

namespace kernel::vm
{
namespace
{
    virt_addr_t generate_vdso_mapping_base()
    {
        // TODO: randomize this
        return virt_addr_t(0x800000000000 - get_vdso_vmo()->length() * 2);
    }
}

util::intrusive_ptr<vas> create_vas(bool randomly_map_vdso)
{
    auto ret = util::make_intrusive<vas>(vas::_key_t{});

    ret->_asid = arch::vm::clone_upper_half();

    if (randomly_map_vdso)
    {
        auto vdso_base = generate_vdso_mapping_base();
        ret->map_vmo(get_vdso_vmo(), vdso_base, flags::user);
    }

    return ret;
}

util::intrusive_ptr<vas> adopt_existing_asid(phys_addr_t asid)
{
    auto ret = util::make_intrusive<vas>(vas::_key_t{});

    ret->_asid = asid;

    return ret;
}

vas::~vas()
{
    PANIC("this requires an amount of work tbh");
}

phys_addr_t vas::get_asid() const
{
    return _asid;
}

bool vas::claim_for_process()
{
    std::lock_guard _(_lock);

    if (_was_claimed_for_process)
    {
        return false;
    }

    _was_claimed_for_process = true;
    return true;
}

util::intrusive_ptr<vmo_mapping> vas::map_vmo(
    util::intrusive_ptr<vmo> vm_object,
    virt_addr_t mapping_base,
    flags fl)
{
    // this is fine because in the future all this will do will be put the mapping into a tree
    // (once on demand mapping is a thing, which will also require a "commit" function on a mapping object)
    std::lock_guard _(_lock);

    auto page_size = arch::vm::page_sizes[vm_object->page_alignment_level()];
    auto alignment_mask = page_size - 1;

    if (mapping_base.value() & alignment_mask)
    {
        PANIC(
            "tried to map a VMO aligned to page level {} (page size: {}) to an address not aligned to the "
            "page alignment ({:#018x})!",
            vm_object->page_alignment_level(),
            page_size,
            mapping_base.value());
    }

    auto mapping_end = mapping_base + vm_object->length();

    auto it = _mappings.find(address_range{ mapping_base, mapping_end });
    if (it != _mappings.end())
    {
        PANIC(
            "tried to map a VMO at ({:#018x}, {:#018x}), which is already occupied by another VMO ({:#018x}, "
            "{:#018x}) in this VAS",
            mapping_base.value(),
            mapping_end.value(),
            it->range().start.value(),
            it->range().end.value());
    }

    auto mapping = util::make_intrusive<vmo_mapping>(this, mapping_base, mapping_end, vm_object, fl);
    _mappings.insert(mapping);

    switch (vm_object->type())
    {
        case vmo_type::physical:
            arch::vm::map_physical(
                this, mapping_base, mapping_base + vm_object->length(), vm_object->base(), fl);
            break;

        case vmo_type::sparse:
            for (auto && element : vm_object->sparse_elements())
            {
                if (!element.backing_address)
                {
                    PANIC(
                        "mapping uncommitted sparse VMOs is not supported yet (TODO after on-demand "
                        "mapping)");
                }

                arch::vm::map_physical(
                    this,
                    mapping_base + element.offset,
                    mapping_base + element.offset + page_size,
                    *element.backing_address,
                    fl);
            }
            break;

        default:
            PANIC("unknown vmo type!");
    }

    if (!_vdso_mapping && vm_object == get_vdso_vmo())
    {
        _vdso_mapping = mapping;
    }

    return mapping;
}

void vas::unmap(vmo_mapping * mapping)
{
    auto lock = mapping->lock();
    std::lock_guard _(_lock);

    arch::vm::unmap(this, mapping->range().start, mapping->range().end, false);

    mapping->release(lock);
}

std::optional<std::shared_lock<std::shared_mutex>> vas::lock_address_range(
    virt_addr_t start,
    virt_addr_t end,
    bool rw)
{
    std::lock_guard _(_lock);

    auto it = _mappings.find(address_range{ start, end });
    if (it == _mappings.end())
    {
        return {};
    }

    if (rw && it->has_flags(flags::read_only))
    {
        return {};
    }

    return { it->shared_lock() };
}

rose::syscall::result vas::syscall_rose_vas_create_handler(
    kernel_caps_t *,
    std::uintptr_t * token_ptr,
    rose::syscall::vdso_mapping_info * vdso_info)
{
    auto new_vas = create_vas(vdso_info != nullptr);

    if (vdso_info)
    {
        auto maybe_base = new_vas->get_vdso_base();
        if (!maybe_base)
        {
            PANIC("couldn't find the base of vdso mapping in a VAS created with a random vdso mapping");
        }

        vdso_info->base = maybe_base->value();
        vdso_info->length = vm::get_vdso_vmo()->length();
    }

    auto handle = create_handle(
        std::move(new_vas),
        rose::syscall::permissions::read | rose::syscall::permissions::write
            | rose::syscall::permissions::transfer | rose::syscall::permissions::clone
            | rose::syscall::permissions::create_mapping);

    *token_ptr = arch::cpu::get_core_local_storage()
                     ->current_thread->get_container()
                     ->register_for_token(std::move(handle))
                     .value();

    return rose::syscall::result::ok;
}

rose::syscall::result vas::syscall_rose_mapping_create_handler(
    vas * vas,
    vmo * vmo,
    std::uintptr_t address,
    std::uintptr_t flags,
    std::uintptr_t * token)
{
    if (flags != 0)
    {
        PANIC("TODO: rose_mapping_create: non zero flags");
    }

    // TODO: remove when on demand mapping is supported
    vmo->commit_all();

    auto mapping = vas->map_vmo(util::intrusive_ptr(vmo), virt_addr_t(address), flags::user);
    auto handle = create_handle(std::move(mapping));

    *token = arch::cpu::get_core_local_storage()
                 ->current_thread->get_container()
                 ->register_for_token(std::move(handle))
                 .value();

    return rose::syscall::result::ok;
}
}
