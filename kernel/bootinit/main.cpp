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

#include "../vdso/include/rose/syscall/early.h"
#include "../vdso/include/rose/syscall/typedefs.h"

[[gnu::section(".bootinit_entry")]] extern "C" int bootinit_main(rose::syscall::token_t mailbox)
{
    for (;;)
    {
        asm volatile("" ::: "memory");
    }

    rose::syscall::early_log("hello world from userspace!");

    (void)mailbox;
}
