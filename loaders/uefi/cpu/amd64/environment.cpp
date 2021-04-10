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

#include "environment.h"
#include "paging.h"

extern "C" void load_gdt();

namespace efi_loader::inline amd64
{
void prepare_environment()
{
    asm volatile("cli;" ::: "memory");
    load_gdt();
    asm volatile("movq %0, %%cr3;" ::"r"(get_cr3_value()) : "memory");
}
}
