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

#include "vm.h"

#include "../arch/vm.h"

#include <boot-constants.h>

namespace
{
// TODO: atomic
std::uint64_t vm_space_top = boot_protocol::kernel_base;
}

namespace kernel::vm
{
virt_addr_t allocate_address_range(std::size_t size)
{
    auto real_size = size + 4095;
    real_size &= ~4095ull;

    auto ret = vm_space_top -= real_size;

    return virt_addr_t(ret);
}
}
