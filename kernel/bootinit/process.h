/*
 * Copyright © 2022 Michał 'Griwes' Dominiak
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

#include <archive/cpio.h>
#include <elf/elf.h>

namespace bootinit::facts
{
extern std::uintptr_t acceptor_mailbox_token;
extern std::uintptr_t kernel_caps_token;
extern std::uintptr_t self_vas_token;
extern std::uintptr_t vdso_size;
}

namespace bootinit::process
{
struct create_process_result
{
    std::uintptr_t process_token;
    std::uintptr_t vas_token;
    std::uintptr_t protocol_mailbox_token;
};

create_process_result create_process(
    const archive::cpio & initrd,
    std::string_view filename,
    std::string_view name);
}
