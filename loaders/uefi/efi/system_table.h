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

#include "types.h"

namespace efi_loader
{
struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

struct EFI_RUNTIME_SERVICES;
struct EFI_BOOT_SERVICES;

struct EFI_CONFIGURATION_TABLE;

struct EFI_SYSTEM_TABLE
{
    EFI_TABLE_HEADER header;
    char16_t * firmware_vendor;
    std::uint32_t firmware_revision;
    EFI_HANDLE con_in_handle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL * con_in;
    EFI_HANDLE con_out_handle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL * con_out;
    EFI_HANDLE stderr_handle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL * stderr;

    EFI_RUNTIME_SERVICES * runtime_services;
    EFI_BOOT_SERVICES * boot_services;

    std::size_t number_of_table_entries;
    EFI_CONFIGURATION_TABLE * configuration_table;
};
} // namespace efi_loader
