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

#include <memory>

#include "../cpu/cpuid.h"
#include "../efi/types.h"
#include "path.h"

namespace efi_loader
{
using file_buffer = std::unique_ptr<char[]>;

path locate_source_directory(EFI_HANDLE);
file_buffer load_file(const path &);
file_buffer load_kernel(const path &, const cpu_capabilities &);
file_buffer load_initrd(const path &, const cpu_capabilities &);
} // namespace efi_loader
