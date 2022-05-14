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

#include "../include/elf/elf.h"
#include "../include/elf/types.h"

namespace elf
{
std::string_view elf_image::get_os_abi_description() const
{
    switch (_header->ident[ident::field::os_abi])
    {
        case ident::os_abi::sysv:
            return "System V";
        case ident::os_abi::hpux:
            return "HP-UX";
        case ident::os_abi::netbsd:
            return "NetBSD";
        case ident::os_abi::linux:
            return "Linux";
        case ident::os_abi::hurd:
            return "Hurd";
        case ident::os_abi::solaris:
            return "Solaris";
        case ident::os_abi::aix:
            return "AIX";
        case ident::os_abi::irix:
            return "IRIX";
        case ident::os_abi::freebsd:
            return "FreeBSD";
        case ident::os_abi::tru64:
            return "Tru64";
        case ident::os_abi::novell_modesto:
            return "Novell Modesto";
        case ident::os_abi::openbsd:
            return "OpenBSD";
        case ident::os_abi::openvms:
            return "OpenVMS";
        case ident::os_abi::standalone:
            return "standalone";
    }

    return "<unknown>";
}

std::string_view elf_image::get_type_description() const
{
    switch (_header->type)
    {
        case header_type::none:
            return "none";
        case header_type::relocatable:
            return "relocatable file";
        case header_type::executable:
            return "executable file";
        case header_type::shared_object:
            return "shared object";
    }

    return "<unknown>";
}

int elf_image::get_abi_version() const
{
    return _header->ident[ident::field::abi_version];
}

try_parse_result try_parse(const char * base, std::size_t length)
{
    if (length < sizeof(elf::header))
    {
        return { std::nullopt, "image is smaller than the size of an elf header", 0 };
    }

    auto header = reinterpret_cast<const elf::header *>(base);

    if (header->ident[ident::field::magic0] != '\x7f' || header->ident[ident::field::magic1] != 'E'
        || header->ident[ident::field::magic2] != 'L' || header->ident[ident::field::magic3] != 'F')
    {
        return { std::nullopt, "image does not have a valid header magic number", 0 };
    }

    if (header->ident[ident::field::class_] != ident::class_64)
    {
        return { std::nullopt, "only Elf64 files are supported", 0 };
    }

    if (header->ident[ident::field::data_encoding] != ident::data_lsb)
    {
        return { std::nullopt, "only little-endian files are supported", 0 };
    }

    if (header->type != header_type::executable && header->type != header_type::shared_object)
    {
        return { std::nullopt, "only executable files and shared objects are supported", 0 };
    }

    if (header->machine != machine_type::amd64)
    {
        return { std::nullopt, "only AMD64 files are supported", 0 };
    }

    if (header->version != 1)
    {
        return { std::nullopt, "only version 1 ELF files are supported", 0 };
    }

    if (header->phoff + header->phentsize * header->phnum > length)
    {
        return { std::nullopt, "the program header table extends beyond the image limit", 0 };
    }

    if (header->shoff + header->shentsize * header->shnum > length)
    {
        return { std::nullopt, "the section header table extends beyond the image limit", 0 };
    }

    elf_image ret;
    ret._header = header;

    std::optional<program_header> dynamic;

    for (auto && program_header : ret.program_headers())
    {
        switch (program_header.type())
        {
            case segment_types::load:
                break;

            case segment_types::dynamic:
                if (!dynamic)
                {
                    dynamic = program_header;
                }
                else
                {
                    return { std::nullopt, "found more than one DYNAMIC segment", program_header.offset() };
                }
                break;

            case segment_types::note:
                break;

            case segment_types::phdr:
                break;

            case segment_types::gnu_stack:
                break;

            case segment_types::gnu_relro:
                break;

            default:
                return { std::nullopt, "unsupported program header type", program_header.offset() };
        }
    }

    if (dynamic)
    {
        ret._dynamic_base = base + dynamic->offset();
        auto dynamic_next = ret._dynamic_base;

        std::size_t dynamic_count = 0;

        std::optional<std::uintptr_t> preinit;
        std::optional<std::size_t> preinit_size;
        std::optional<std::uintptr_t> init;
        std::optional<std::size_t> init_size;
        std::optional<std::uintptr_t> fini;
        std::optional<std::size_t> fini_size;

        while (true)
        {
            bool end = false;

            auto dynamic_entry = reinterpret_cast<const elf::dynamic_entry *>(dynamic_next);
            switch (dynamic_entry->tag)
            {
                case dynamic_entry_types::null:
                    end = true;
                    break;

                case dynamic_entry_types::needed:
                    break;

                case dynamic_entry_types::strtab:
                    ret._strtab = base + dynamic_entry->un.ptr;
                    break;

                case dynamic_entry_types::symtab:
                    ret._symtab = reinterpret_cast<const symbol *>(base + dynamic_entry->un.ptr);
                    break;

                case dynamic_entry_types::rela:
                    ret._relocations = reinterpret_cast<const rela *>(base + dynamic_entry->un.ptr);
                    break;

                case dynamic_entry_types::rela_size:
                    ret._relocations_count = dynamic_entry->un.val / sizeof(rela);
                    break;

                case dynamic_entry_types::rela_entry_size:
                    if (dynamic_entry->un.val != sizeof(rela))
                    {
                        return { std::nullopt, "unsupported RELA value", 0 };
                    }
                    break;

                case dynamic_entry_types::str_size:
                    break;

                case dynamic_entry_types::sym_entry_size:
                    if (dynamic_entry->un.val != sizeof(symbol))
                    {
                        return { std::nullopt, "unsupported SYMENT value", 0 };
                    }
                    break;

                case dynamic_entry_types::soname:
                    break;

                case dynamic_entry_types::debug:
                    break;

                case dynamic_entry_types::bind_now:
                    break;

                case dynamic_entry_types::init_array:
                    init = dynamic_entry->un.ptr;
                    break;

                case dynamic_entry_types::init_array_size:
                    init_size = dynamic_entry->un.val;
                    break;

                case dynamic_entry_types::flags:
                    break;

                case dynamic_entry_types::gnu_hash:
                    break;

                case dynamic_entry_types::rela_count:
                    break;

                case dynamic_entry_types::flags_1:
                    break;

                default:
                    return { std::nullopt, "unsupported DYNAMIC entry tag", dynamic_next - base };
            }

            if (end)
            {
                break;
            }

            dynamic_next += sizeof(elf::dynamic_entry);
            ++dynamic_count;
        }

        if (preinit && !preinit_size)
        {
            return { std::nullopt, "PREINIT DYNAMIC entry present without PREINIT_SIZE DYANMIC entry", 0 };
        }
        if (!preinit && preinit_size)
        {
            return { std::nullopt, "PREINIT_SIZE DYNAMIC entry present without PREINIT DYANMIC entry", 0 };
        }

        if (init && !init_size)
        {
            return { std::nullopt, "INIT DYNAMIC entry present without INIT_SIZE DYANMIC entry", 0 };
        }
        if (!init && init_size)
        {
            return { std::nullopt, "INIT_SIZE DYNAMIC entry present without INIT DYANMIC entry", 0 };
        }

        if (fini && !fini_size)
        {
            return { std::nullopt, "FINI DYNAMIC entry present without FINI_SIZE DYANMIC entry", 0 };
        }
        if (!fini && fini_size)
        {
            return { std::nullopt, "FINI_SIZE DYNAMIC entry present without FINI DYANMIC entry", 0 };
        }

        if (preinit)
        {
            ret._preinit_desc = function_table_descriptor{ *preinit, *preinit_size };
        }
        if (init)
        {
            ret._init_desc = function_table_descriptor{ *init, *init_size };
        }
        if (fini)
        {
            ret._fini_desc = function_table_descriptor{ *fini, *fini_size };
        }

        ret._dynamic_count = dynamic_count;
    }

    for (auto && section : ret.sections())
    {
        if (section.offset() + section.length() > length)
        {
            return { std::nullopt, "a section extends beyond the image limit", section.offset() };
        }

        switch (section.type())
        {
            case section_types::progbits:
                break;

            case section_types::symtab:
                break;

            case section_types::strtab:
                break;

            case section_types::rela:
                break;

            case section_types::hash:
                return { std::nullopt,
                         "binaries containing .hash are not supported; use .gnu.hash instead",
                         section.offset() };

            case section_types::dynamic:
                break;

            case section_types::note:
                break;

            case section_types::nobits:
                break;

            case section_types::dynsym:
                if (base + section.offset() == reinterpret_cast<const char *>(ret._symtab))
                {
                    ret._symtab_size = section.length() / sizeof(symbol);
                }
                break;

            case section_types::init_array:
                break;

            case section_types::gnu_hash:
                break;

            default:
                return { std::nullopt, "unsupported section type", section.offset() };
        }
    }

    if (ret._symtab && !ret._symtab_size)
    {
        return { std::nullopt, "found a SYMTAB, but not its corresponding section", 0 };
    }

    return { ret, "", 0 };
}

namespace
{
    rela _rel_to_rela(rel r)
    {
        rela ret;
        ret.offset = r.offset;
        ret.info = r.info;
        ret.addend = 0;
        return ret;
    }
}

bool relocation::needs_symbol() const
{
    auto rela =
        _reloc_entry.index() == 0 ? _rel_to_rela(*std::get<0>(_reloc_entry)) : *std::get<1>(_reloc_entry);

    switch (rtype(rela.info))
    {
        case relocs::amd64::R_X86_64_RELATIVE:
            return false;

        default:
            return true;
    }
}

std::string_view relocation::symbol_name() const
{
    auto symbol_index =
        rsym(_reloc_entry.index() == 0 ? std::get<0>(_reloc_entry)->info : std::get<1>(_reloc_entry)->info);
    auto & symbol = _symtab[symbol_index];
    if (symbol.name)
    {
        return { _strtab + symbol.name };
    }
    return "";
}

calculate_and_write_result relocation::calculate_and_write(
    std::uintptr_t symbol_value,
    std::uintptr_t image_vbase,
    std::uintptr_t segment_vaddr,
    char * segment_base) const
{
    auto rela =
        _reloc_entry.index() == 0 ? _rel_to_rela(*std::get<0>(_reloc_entry)) : *std::get<1>(_reloc_entry);

    calculate_and_write_result ret;
    ret.target_address = image_vbase + segment_vaddr + rela.offset;

    // TODO: switch on machine type here
    // TODO: throw instead of ud2 in hosted environments

    switch (rtype(rela.info))
    {
        case relocs::amd64::R_X86_64_GLOB_DAT:
            ret.relocation_value =
                *reinterpret_cast<std::uint64_t *>(segment_base + rela.offset - segment_vaddr) = symbol_value;
            break;

        case relocs::amd64::R_X86_64_RELATIVE:
            ret.relocation_value =
                *reinterpret_cast<std::uint64_t *>(segment_base + rela.offset - segment_vaddr) =
                    image_vbase + rela.addend;
            break;

        default:
            asm volatile("ud2");
    }

    return ret;
}
}
