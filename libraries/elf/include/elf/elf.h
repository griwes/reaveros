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

#include <optional>
#include <string_view>
#include <variant>

#include "types.h"

namespace elf
{
class elf_image;
struct try_parse_result;

try_parse_result try_parse(const char * base, std::size_t length);

class section
{
public:
    section(const section_header * header) : _header(header)
    {
    }

    std::ptrdiff_t offset() const
    {
        return _header->offset;
    }

    std::size_t length() const
    {
        return _header->size;
    }

    section_types::type type() const
    {
        return static_cast<section_types::type>(_header->type);
    }

private:
    const section_header * _header;
};

class section_iterator
{
public:
    section_iterator(const char * section_header_table, half entry_size, std::size_t index = 0)
        : _section_header_table(section_header_table), _index(index), _entry_size(entry_size)
    {
    }

    section_iterator & operator++()
    {
        ++_index;
        return *this;
    }

    section operator*() const
    {
        return section(
            reinterpret_cast<const section_header *>(_section_header_table + _index * _entry_size));
    }

    friend bool operator==(section_iterator, section_iterator) = default;

private:
    const char * _section_header_table;
    std::size_t _index;
    half _entry_size;
};

class sections
{
public:
    sections(const char * section_header_table, half entry_size, std::size_t table_size)
        : _section_header_table(section_header_table), _table_size(table_size), _entry_size(entry_size)
    {
    }

    section_iterator begin() const
    {
        return { _section_header_table, _entry_size, 1 };
    }

    section_iterator end() const
    {
        return { _section_header_table, _entry_size, _table_size };
    }

    auto size() const
    {
        return _table_size;
    }

private:
    const char * _section_header_table;
    std::size_t _table_size;
    half _entry_size;
};

class program_header
{
public:
    program_header(const program_header_entry * entry) : _entry(entry)
    {
    }

    std::ptrdiff_t offset() const
    {
        return _entry->offset;
    }

    segment_types::type type() const
    {
        return static_cast<segment_types::type>(_entry->type);
    }

    std::size_t size() const
    {
        return _entry->memsz;
    }

    std::size_t file_size() const
    {
        return _entry->filesz;
    }

    std::uintptr_t virtual_address() const
    {
        return _entry->vaddr;
    }

private:
    const program_header_entry * _entry;
};

class program_header_iterator
{
public:
    program_header_iterator(const char * program_header_table, half entry_size, std::size_t index = 0)
        : _program_header_table(program_header_table), _index(index), _entry_size(entry_size)
    {
    }

    program_header_iterator & operator++()
    {
        ++_index;
        return *this;
    }

    program_header operator*() const
    {
        return program_header(
            reinterpret_cast<const program_header_entry *>(_program_header_table + _index * _entry_size));
    }

    friend bool operator==(program_header_iterator, program_header_iterator) = default;

private:
    const char * _program_header_table;
    std::size_t _index;
    half _entry_size;
};

class program_headers
{
public:
    program_headers(const char * program_header_table, half entry_size, std::size_t table_size)
        : _program_header_table(program_header_table), _table_size(table_size), _entry_size(entry_size)
    {
    }

    program_header_iterator begin() const
    {
        return { _program_header_table, _entry_size, 0 };
    }

    program_header_iterator end() const
    {
        return { _program_header_table, _entry_size, _table_size };
    }

    auto size() const
    {
        return _table_size;
    }

private:
    const char * _program_header_table;
    std::size_t _table_size;
    half _entry_size;
};

class dynamic_entry_wrapper
{
public:
    dynamic_entry_wrapper(const dynamic_entry * entry, const char * strtab) : _entry(entry), _strtab(strtab)
    {
    }

    dynamic_entry_types::type type() const
    {
        return static_cast<dynamic_entry_types::type>(_entry->tag);
    }

    std::string_view needed_name() const
    {
        if (_entry->tag != dynamic_entry_types::needed)
        {
            return "<not a NEEDED entry>"; // TODO: do this more sanely somehow
        }

        return { _strtab + _entry->un.val };
    }

    std::uintptr_t value() const
    {
        return _entry->un.val;
    }

private:
    const dynamic_entry * _entry;
    const char * _strtab;
};

class dynamic_entry_iterator
{
public:
    dynamic_entry_iterator(const char * base, std::size_t index, const char * strtab)
        : _base(base), _index(index), _strtab(strtab)
    {
    }

    dynamic_entry_iterator & operator++()
    {
        ++_index;
        return *this;
    }

    dynamic_entry_wrapper operator*() const
    {
        return { reinterpret_cast<const dynamic_entry *>(_base) + _index, _strtab };
    }

    friend bool operator==(dynamic_entry_iterator, dynamic_entry_iterator) = default;

private:
    const char * _base;
    std::size_t _index;
    const char * _strtab;
};

class dynamic_entries
{
public:
    dynamic_entries(const char * dynamic_base, std::size_t dynamic_size, const char * strtab)
        : _dynamic_base(dynamic_base), _dynamic_size(dynamic_size), _strtab(strtab)
    {
    }

    dynamic_entry_iterator begin() const
    {
        return { _dynamic_base, 0, _strtab };
    }

    dynamic_entry_iterator end() const
    {
        return { _dynamic_base, _dynamic_size, _strtab };
    }

private:
    const char * _dynamic_base;
    std::size_t _dynamic_size;
    const char * _strtab;
};

using relocation_ptr = std::variant<const rel *, const rela *>;

class relocation
{
public:
    relocation(relocation_ptr reloc_entry, const symbol * symtab, const char * strtab)
        : _reloc_entry(reloc_entry), _symtab(symtab), _strtab(strtab)
    {
    }

    std::uintptr_t offset() const
    {
        return _reloc_entry.index() == 0 ? std::get<0>(_reloc_entry)->offset
                                         : std::get<1>(_reloc_entry)->offset;
    }

    std::string_view symbol_name() const;
    void calculate_and_write(std::uintptr_t symbol_value, std::uintptr_t segment_vaddr, char * segment_base)
        const;

private:
    relocation_ptr _reloc_entry;
    const symbol * _symtab;
    const char * _strtab;
};

class relocation_iterator
{
public:
    relocation_iterator(
        relocation_ptr relocations,
        std::size_t index,
        const symbol * symtab,
        const char * strtab)
        : _relocations(relocations), _index(index), _symtab(symtab), _strtab(strtab)
    {
    }

    relocation_iterator & operator++()
    {
        ++_index;
        return *this;
    }

    relocation operator*() const
    {
        if (_relocations.index() == 0)
        {
            return { std::get<0>(_relocations) + _index, _symtab, _strtab };
        }
        else
        {
            return { std::get<1>(_relocations) + _index, _symtab, _strtab };
        }
    }

    friend bool operator==(relocation_iterator, relocation_iterator) = default;

private:
    relocation_ptr _relocations;
    std::size_t _index;
    const symbol * _symtab;
    const char * _strtab;
};

class relocations
{
public:
    relocations(
        relocation_ptr relocations,
        std::size_t relocation_count,
        const symbol * symtab,
        const char * strtab)
        : _relocations(relocations), _relocation_count(relocation_count), _symtab(symtab), _strtab(strtab)
    {
    }

    relocation_iterator begin() const
    {
        return { _relocations, 0, _symtab, _strtab };
    }

    relocation_iterator end() const
    {
        return { _relocations, _relocation_count, _symtab, _strtab };
    }

    std::size_t size() const
    {
        return _relocation_count;
    }

private:
    relocation_ptr _relocations;
    std::size_t _relocation_count;
    const symbol * _symtab;
    const char * _strtab;
};

class symbol_wrapper
{
public:
    symbol_wrapper(const symbol * symtab_entry, const char * strtab)
        : _symbol_entry(symtab_entry), _strtab(strtab)
    {
    }

    std::string_view name() const
    {
        if (_symbol_entry->name)
        {
            return { _strtab + _symbol_entry->name };
        }

        return "";
    }

    std::ptrdiff_t value() const
    {
        return _symbol_entry->value;
    }

    bool is_defined() const
    {
        return _symbol_entry->value != 0;
    }

private:
    const symbol * _symbol_entry;
    const char * _strtab;
};

class symbol_iterator
{
public:
    symbol_iterator(const symbol * symtab, std::size_t index, const char * strtab)
        : _symtab(symtab), _index(index), _strtab(strtab)
    {
    }

    symbol_iterator & operator++()
    {
        ++_index;
        return *this;
    }

    symbol_wrapper operator*() const
    {
        return { _symtab + _index, _strtab };
    }

    friend bool operator==(symbol_iterator, symbol_iterator) = default;

private:
    const symbol * _symtab;
    std::size_t _index;
    const char * _strtab;
};

class symbols
{
public:
    symbols(const symbol * symtab, std::size_t symtab_size, const char * strtab)
        : _symtab(symtab), _symtab_size(symtab_size), _strtab(strtab)
    {
    }

    symbol_iterator begin() const
    {
        return { _symtab, 1, _strtab };
    }

    symbol_iterator end() const
    {
        return { _symtab, _symtab_size, _strtab };
    }

private:
    const symbol * _symtab;
    std::size_t _symtab_size;
    const char * _strtab;
};

class elf_image
{
public:
    friend try_parse_result try_parse(const char * base, std::size_t length);

    std::string_view get_os_abi_description() const;
    std::string_view get_type_description() const;
    int get_abi_version() const;

    std::uintptr_t get_entry_point() const
    {
        return _header->entry;
    }

    sections sections() const
    {
        return { _base() + _header->shoff, _header->shentsize, _header->shnum };
    }

    program_headers program_headers() const
    {
        return { _base() + _header->phoff, _header->phentsize, _header->phnum };
    }

    dynamic_entries dynamic_entries() const
    {
        return { _dynamic_base, _dynamic_count, _strtab };
    }

    relocations relocations() const
    {
        return { _relocations, _relocations_count, _symtab, _strtab };
    }

    symbols symbols() const
    {
        return { _symtab, _symtab_size, _strtab };
    }

private:
    const char * _base() const
    {
        return reinterpret_cast<const char *>(_header);
    }

    const header * _header = nullptr;
    const char * _dynamic_base = nullptr;
    std::size_t _dynamic_count = 0;
    const char * _strtab = nullptr;
    relocation_ptr _relocations;
    std::size_t _relocations_count = 0;
    const symbol * _symtab = nullptr;
    std::size_t _symtab_size = 0;
};

struct try_parse_result
{
    std::optional<elf_image> image;
    std::string_view error_message;
    std::ptrdiff_t offset;
};
};
