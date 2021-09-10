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

#ifdef __amd64__

#include "amd64/memory/vm.h"

#define arch_namespace amd64

#else

#error "unknown architecture"

#endif

namespace kernel::arch::vm
{
using arch_namespace::vm::page_size_count;
using arch_namespace::vm::page_sizes;

using arch_namespace::vm::get_asid;
using arch_namespace::vm::set_asid;

using arch_namespace::vm::map_physical;
using arch_namespace::vm::unmap;

using arch_namespace::vm::clone_upper_half;
}

#undef arch_namespace
