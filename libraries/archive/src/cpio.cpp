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

#include "../include/archive/cpio.h"

#include <charconv>

namespace archive
{
namespace
{
    struct cpio_header
    {
        char magic[6];
        char inode[8];
        char mode[8];
        char uid[8];
        char gid[8];
        char nlink[8];
        char mtime[8];
        char filesize[8];
        char devmajor[8];
        char devminor[8];
        char rdevmajor[8];
        char rdevminor[8];
        char namesize[8];
        char check[8];
    };

    struct get_entry_result;

    struct header
    {
        cpio_header * hdr;
        std::uintptr_t namesize = 0;
        std::uintptr_t name_padding = 0;
        std::uintptr_t filesize = 0;
        std::uintptr_t file_padding = 0;
        std::uintptr_t mode = 0;

        get_entry_result next(char * limit);

        bool is_file()
        {
            return (mode & 0100000) == 0100000;
        }

        std::string_view filename()
        {
            return std::string_view(reinterpret_cast<char *>(hdr) + sizeof(cpio_header), namesize);
        }

        std::string_view contents()
        {
            return std::string_view(
                reinterpret_cast<char *>(hdr) + sizeof(cpio_header) + namesize + name_padding, filesize);
        }
    };

    struct get_entry_result
    {
        std::optional<header> entry;
        std::string_view error_message;
        char * header_base;
    };

    get_entry_result get_entry(char * base, char * limit)
    {
        if (base + sizeof(cpio_header) >= limit)
        {
            return { std::nullopt, "end of entry would be past the limit of the archive", base };
        }

        auto hdr = reinterpret_cast<cpio_header *>(base);

        if (std::string_view(hdr->magic, 6) != "070701")
        {
            return { std::nullopt, "magic did not match", base };
        }

        std::uintptr_t mode;
        auto result = std::from_chars(std::begin(hdr->mode), std::end(hdr->mode), mode, 16);

        if (result.ec != std::errc() || result.ptr != std::end(hdr->mode))
        {
            return { std::nullopt, "failed to parse file mode", base };
        }

        std::uintptr_t namesize;
        result = std::from_chars(std::begin(hdr->namesize), std::end(hdr->namesize), namesize, 16);

        if (result.ec != std::errc() || result.ptr != std::end(hdr->namesize))
        {
            return { std::nullopt, "failed to parse namesize", base };
        }

        std::uintptr_t filesize;
        result = std::from_chars(std::begin(hdr->filesize), std::end(hdr->filesize), filesize, 16);

        if (result.ec != std::errc() || result.ptr != std::end(hdr->filesize))
        {
            return { std::nullopt, "failed to parse filesize", base };
        }

        auto name_padding = (4 - (sizeof(cpio_header) + namesize) % 4) % 4;
        auto file_padding = (4 - filesize % 4) % 4;

        if (reinterpret_cast<char *>(hdr) + sizeof(cpio_header) + namesize + name_padding + filesize
                + file_padding
            >= limit)
        {
            return { std::nullopt, "end of file would be past the limit of the archive", base };
        }

        return { header{ .hdr = hdr,
                         .namesize = namesize,
                         .name_padding = name_padding,
                         .filesize = filesize,
                         .file_padding = file_padding,
                         .mode = mode },
                 "",
                 nullptr };
    }

    get_entry_result header::next(char * limit)
    {
        return get_entry(
            reinterpret_cast<char *>(hdr) + sizeof(cpio_header) + namesize + name_padding + filesize
                + file_padding,
            limit);
    }
}

try_cpio_result try_cpio(char * base, std::size_t length)
{
    std::size_t size = 0;
    std::size_t index = 0;

    auto end = base + length;

    auto result = get_entry(base, end);

    do
    {
        if (!result.entry)
        {
            return { std::nullopt, result.error_message, result.header_base - base };
        }

        if (result.entry->is_file())
        {
            ++size;
        }

        if (result.entry->filename() == "TRAILER!!!")
        {
            break;
        }

        result = result.entry->next(end);
        ++index;
    } while (true);

    cpio ret;
    ret._size = size;
    ret._base = base;
    ret._limit = end;
    return { ret, "", 0 };
}

std::optional<std::string_view> cpio::operator[](std::string_view filename) const
{
    auto result = get_entry(_base, _limit);

    do
    {
        if (!result.entry)
        {
            return std::nullopt;
        }

        if (result.entry->is_file() && result.entry->filename() == filename)
        {
            return result.entry->contents();
        }

        if (result.entry->filename() == "TRAILER!!!")
        {
            return std::nullopt;
        }

        result = result.entry->next(_limit);
    } while (true);
}
}
