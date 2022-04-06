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

#include <cstddef>
#include <optional>
#include <string_view>

#ifndef __ROSE_FREESTANDING
#warning "maybe reimplement this to support non-freestanding binaries"
#endif

namespace archive
{
class cpio;
struct try_cpio_result;

try_cpio_result try_cpio(const char * base, std::size_t length);

class cpio
{
public:
    std::size_t size() const
    {
        return _size;
    }

    std::optional<std::string_view> operator[](std::string_view filename) const;

    friend try_cpio_result try_cpio(const char *, std::size_t);

private:
    std::size_t _size;

    const char * _base;
    const char * _limit;
};

struct try_cpio_result
{
    std::optional<cpio> archive;
    std::string_view error_message;
    std::ptrdiff_t header_offset;
};
}
