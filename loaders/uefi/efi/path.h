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

#include <string_view>

namespace efi_loader
{
struct EFI_DEVICE_PATH_PROTOCOL;

class path
{
public:
    path(EFI_DEVICE_PATH_PROTOCOL *);
    path(std::string_view);
    ~path();

    path(const path &) = delete;
    path(path &&) = default;
    path & operator=(const path &) = delete;
    path & operator=(path &&) = default;

    void print() const;

    const char16_t * string() const
    {
        return _buffer;
    }

    path operator/(const char16_t *) const;

private:
    char16_t * _buffer;
};
} // namespace efi_loader
