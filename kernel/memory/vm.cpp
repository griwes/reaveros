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

#include "vm.h"

#include "../arch/vm.h"
#include "../util/intrusive_ptr.h"
#include "vmo.h"

#include <boot-constants.h>

#include <atomic>

namespace kernel::vm
{
namespace
{
    std::atomic<std::uint64_t> vm_space_top = boot_protocol::kernel_base - 4096;
    util::intrusive_ptr<vmo> vdso_vmo{};
}

virt_addr_t allocate_address_range(std::size_t size)
{
    auto real_size = size + 4095;
    real_size &= ~4095ull;

    auto old = vm_space_top.fetch_sub(real_size);

    return virt_addr_t(old - real_size);
}

void set_vdso_vmo(util::intrusive_ptr<vmo> vdso)
{
    if (vdso_vmo)
    {
        PANIC("set_vdso_vmo called more than once!");
    }

    vdso_vmo = vdso;
}

util::intrusive_ptr<vmo> get_vdso_vmo()
{
    if (!vdso_vmo)
    {
        PANIC("get_vdso_vmo called before set_vdso_vmo");
    }

    return vdso_vmo;
}
}
