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

#include <cstddef>
#include <cstdint>

using ctor_t = void (*)();
extern "C" ctor_t __start_ctors;
extern "C" ctor_t __end_ctors;

void __init()
{
    for (auto ctor = &__start_ctors; ctor != &__end_ctors; ++ctor)
    {
        (*ctor)();
    }
}

[[gnu::section(".reaveros_entry")]] extern "C" void kernel_main(
    std::size_t memmap_size,
    std::uintptr_t memmap_entries)
{
    asm volatile("cli; hlt;");
}
