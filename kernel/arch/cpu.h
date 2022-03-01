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

#ifdef __amd64__

#include "amd64/cpu/core.h"
#include "amd64/cpu/cpu.h"

#define arch_namespace amd64

#else

#error "unknown architecture"

#endif

namespace kernel::arch::cpu
{
using arch_namespace::cpu::core;
using arch_namespace::cpu::get_core_count;
using arch_namespace::cpu::idle;
using arch_namespace::cpu::initialize;
using arch_namespace::cpu::switch_to_clean_state;

using arch_namespace::cpu::get_core_local_storage;

using arch_namespace::cpu::get_core_by_id;
using arch_namespace::cpu::get_current_core;
}

#undef arch_namespace
