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

#pragma once

#include "../memory/vas.h"

#include <boot-memmap.h>

#include <memory>

namespace kernel::initrd
{
void initialize(std::size_t memmap_size, boot_protocol::memory_map_entry * memmap);
std::unique_ptr<vm::vas> create_bootinit_vas();

constexpr virt_addr_t bootinit_ip_address(0x10000);
constexpr virt_addr_t bootinit_initrd_address(0x100000);
constexpr virt_addr_t bootinit_top_of_stack(0x80000000);
}
