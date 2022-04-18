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

#include <cstdint>

namespace elf
{
using addr = std::uint64_t;
using off = std::uint64_t;
using half = std::uint16_t;
using word = std::uint32_t;
using sword = std::int32_t;
using xword = std::uint64_t;
using sxword = std::uintptr_t;

namespace ident
{
    namespace field
    {
        enum
        {
            magic0,
            magic1,
            magic2,
            magic3,
            class_,
            data_encoding,
            version,
            os_abi,
            abi_version
        };
    }

    enum ident_class
    {
        class_32 = 1,
        class_64 = 2
    };

    enum data_encoding
    {
        data_lsb = 1,
        data_msb = 2
    };

    namespace os_abi
    {
        enum
        {
            sysv = 0,
            hpux = 1,
            netbsd = 2,
            linux = 3,
            hurd = 4,
            solaris = 6,
            aix = 7,
            irix = 8,
            freebsd = 9,
            tru64 = 0xa,
            novell_modesto = 0xb,
            openbsd = 0xc,
            openvms = 0xd,
            standalone = 255
        };
    }
}

namespace header_type
{
    enum : half
    {
        none = 0,
        relocatable = 1,
        executable = 2,
        shared_object = 3,
        core_file = 4,
        os_specific_low = 0xfe00,
        os_specific_high = 0xfeff,
        cpu_specific_low = 0xff00,
        cpu_specific_high = 0xffff
    };
}

namespace machine_type
{
    enum : half
    {
        unknown = 0,
        amd64 = 0x3e,
        aarch64 = 0xb7,
        riscv = 0xf3
    };
}

struct header
{
    unsigned char ident[16]; /*  ELF  identification  */
    half type;               /*  Object  file  type  */
    half machine;            /*  Machine  type  */
    word version;            /*  Object  file  version  */
    addr entry;              /*  Entry  point  address  */
    off phoff;               /*  Program  header  offset  */
    off shoff;               /*  Section  header  offset  */
    word flags;              /*  Processor-specific  flags  */
    half ehsize;             /*  ELF  header  size  */
    half phentsize;          /*  Size  of  program  header  entry  */
    half phnum;              /*  Number  of  program  header  entries  */
    half shentsize;          /*  Size  of  section  header  entry  */
    half shnum;              /*  Number  of  section  header  entries  */
    half shstrndx;           /*  Section  name  string  table  index  */
};

namespace section_indices
{
    enum : half
    {
        undef = 0,
        cpu_specific_low = 0xff00,
        cpu_specific_high = 0xff1f,
        os_specific_low = 0xff20,
        os_specific_high = 0xff3f,
        absolute = 0xfff1,
        common = 0xfff2
    };
}

namespace section_types
{
    enum type : word
    {
        null,
        progbits,
        symtab,
        strtab,
        rela,
        hash,
        dynamic,
        note,
        nobits,
        rel,
        shlib,
        dynsym,

        os_specific_low = 0x60000000,

        gnu_hash = 0x6ffffff6,

        os_specific_high = 0x6fffffff,

        cpu_specific_low = 0x70000000,

        amd64_unwind = 0x70000001,

        cpu_specific_high = 0x7fffffff
    };
}

namespace section_attributes
{
    enum : xword
    {
        write = 0x1,
        alloc = 0x2,
        execinstr = 0x4,

        os_specific_mask = 0x0f000000,

        amd64_large = 0x10000000,
        cpu_specific_mask = 0xf0000000
    };
};

struct section_header
{
    word name;       /*  Section  name  */
    word type;       /*  Section  type  */
    xword flags;     /*  Section  attributes  */
    addr addr;       /*  Virtual  address  in  memory  */
    off offset;      /*  Offset  in  file  */
    xword size;      /*  Size  of  section  */
    word link;       /*  Link  to  other  section  */
    word info;       /*  Miscellaneous  information  */
    xword addralign; /*  Address  alignment  boundary  */
    xword entsize;   /*  Size  of  entries,  if  section  has  table  */
};

namespace symbol_bindings
{
    enum : unsigned char
    {
        local = 0,
        global = 1 << 4,
        weak = 2 << 4,
        os_specific_low = 10 << 4,
        os_specific_high = 12 << 4,
        cpu_specific_low = 13 << 4,
        cpu_specific_high = 15 << 4
    };
}

namespace symbol_types
{
    enum
    {
        no_type,
        object,
        function,
        section,
        file,
        os_specific_low = 10,
        os_specific_high = 12,
        cpu_specific_low = 13,
        cpu_specific_high = 15
    };
}

struct symbol
{
    word name;           /*  Symbol  name  */
    unsigned char info;  /*  Type  and  Binding  attributes  */
    unsigned char other; /*  Reserved  */
    half shndx;          /*  Section  table  index  */
    addr value;          /*  Symbol  value  */
    xword size;          /*  Size  of  object  (e.g.,  common)  */
};

struct rel
{
    addr offset; /*  Address  of  reference  */
    xword info;  /*  Symbol  index  and  type  of  relocation  */
};

struct rela
{
    addr offset;   /*  Address  of  reference  */
    xword info;    /*  Symbol  index  and  type  of  relocation  */
    sxword addend; /*  Constant  part  of  expression  */
};

constexpr auto rsym(xword info)
{
    return info >> 32;
}

constexpr auto rtype(xword info)
{
    return info & 0xffffffff;
}

constexpr auto rinfo(xword symbol_index, xword type)
{
    return (symbol_index << 32) + (type & 0xfffffffff);
}

namespace segment_types
{
    enum type : word
    {
        null,
        load,
        dynamic,
        interp,
        note,
        shlib,
        phdr,

        os_specific_low = 0x60000000,

        gnu_eh_frame = 0x6474e550,
        gnu_stack = 0x6474e551,
        gnu_relro = 0x6474e552,

        os_specific_high = 0x6fffffff,

        cpu_specific_low = 0x70000000,
        cpu_specific_high = 0x7fffffff
    };
}

namespace segment_attributes
{
    enum : word
    {
        x = 0x1,
        w = 0x2,
        r = 0x4,
        os_specific_mask = 0x00ff0000,
        cpu_specific_mask = 0xff000000
    };
}

struct program_header_entry
{
    word type;    /*  Type  of  segment  */
    word flags;   /*  Segment  attributes  */
    off offset;   /*  Offset  in  file  */
    addr vaddr;   /*  Virtual  address  in  memory  */
    addr paddr;   /*  Reserved  */
    xword filesz; /*  Size  of  segment  in  file  */
    xword memsz;  /*  Size  of  segment  in  memory  */
    xword align;  /*  Alignment  of  segment  */
};

namespace dynamic_entry_types
{
    enum type : sxword
    {
        null,
        needed,
        pltrel_size,
        pltgot,
        hash,
        strtab,
        symtab,
        rela,
        rela_size,
        rela_entry_size,
        str_size,
        sym_entry_size,
        init,
        fini,
        soname,
        rpath,
        symbolic,
        rel,
        rel_size,
        rel_entry_size,
        pltrel,
        debug,
        textrel,
        jmprel,
        bind_now,
        init_array,
        fini_array,
        init_array_size,
        fini_array_size,
        runpath,
        flags,
        encoding,
        preinit_array,
        preinit_array_size,

        os_specific_low = 0x60000000,

        gnu_hash = 0x6ffffef5,
        flags_1 = 0x6ffffffb,

        os_specific_high = 0x6fffffff,

        cpu_specific_low = 0x70000000,
        cpu_specific_high = 0x7fffffff
    };
}

struct dynamic_entry
{
    sxword tag;
    union
    {
        xword val;
        addr ptr;
    } un;
};

constexpr unsigned long hash(const unsigned char * name)
{
    unsigned long h = 0, g;
    while (*name)
    {
        h = (h << 4) + *name++;
        if ((g = h & 0xf0000000))
            h ^= g >> 24;
        h &= 0x0fffffff;
    }
    return h;
}

namespace relocs
{
    namespace amd64
    {
        enum type : word
        {
            R_X86_64_NONE = 0,
            R_X86_64_64 = 1,
            R_X86_64_PC32 = 2,
            R_X86_64_GOT32 = 3,
            R_X86_64_PLT32 = 4,
            R_X86_64_COPY = 5,
            R_X86_64_GLOB_DAT = 6,
            R_X86_64_JUMP_SLOT = 7,
            R_X86_64_RELATIVE = 8,
            R_X86_64_GOTPCREL = 9,
            R_X86_64_32 = 10,
            R_X86_64_32S = 11,
            R_X86_64_16 = 12,
            R_X86_64_PC16 = 13,
            R_X86_64_8 = 14,
            R_X86_64_PC8 = 15,
            R_X86_64_DTPMOD64 = 16,
            R_X86_64_DTPOFF64 = 17,
            R_X86_64_TPOFF64 = 18,
            R_X86_64_TLSGD = 19,
            R_X86_64_TLSLD = 20,
            R_X86_64_DTPOFF32 = 21,
            R_X86_64_GOTTPOFF = 22,
            R_X86_64_TPOFF32 = 23,
            R_X86_64_PC64 = 24,
            R_X86_64_GOTOFF64 = 25,
            R_X86_64_GOTPC32 = 26,
            R_X86_64_SIZE32 = 32,
            R_X86_64_SIZE64 = 33,
            R_X86_64_GOTPC32_TLSDESC = 34,
            R_X86_64_TLSDESC_CALL = 35,
            R_X86_64_TLSDESC = 36,
            R_X86_64_IRELATIVE = 37,
        };
    }
}
}
