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

#include "../arch/vm.h"
#include "addresses.h"
#include "log.h"

#include <archive/cpio.h>
#include <elf/elf.h>

#include <user/meta.h>

using ctor_t = void (*)();
extern "C" ctor_t __start_ctors;
extern "C" ctor_t __end_ctors;

extern "C" void __init()
{
    for (auto ctor = &__start_ctors; ctor != &__end_ctors; ++ctor)
    {
        (*ctor)();
    }
}

extern "C" void __cxa_atexit(void (*)(void *), void *, void *)
{
}

[[noreturn]] extern "C" void __cxa_pure_virtual()
{
    PANIC("Pure virtual method called!");
}

namespace sc = rose::syscall;

namespace
{
std::uintptr_t kernel_caps_token;
std::uintptr_t self_vas_token;
std::uintptr_t vdso_size;

std::uintptr_t top_of_image = bootinit::addresses::top_of_stack.value() + kernel::arch::vm::page_sizes[0];

template<typename T>
struct allocate_array_result
{
    allocate_array_result() = default;

    allocate_array_result(const allocate_array_result &) = delete;

    allocate_array_result(allocate_array_result && other)
        : ptr(std::exchange(other.ptr, nullptr)),
          size(std::exchange(other.size, 0)),
          mapping_token(std::exchange(other.mapping_token, 0)),
          vmo_token(std::exchange(other.vmo_token, 0))
    {
    }

    allocate_array_result & operator=(const allocate_array_result &) = delete;

    allocate_array_result & operator=(allocate_array_result && other)
    {
        if (this == &other)
        {
            return *this;
        }

        if (mapping_token)
        {
            [[maybe_unused]] allocate_array_result<T> tmp(std::move(*this));
        }

        ptr = std::exchange(other.ptr, nullptr);
        size = std::exchange(other.size, 0);
        mapping_token = std::exchange(other.mapping_token, 0);
        vmo_token = std::exchange(other.vmo_token, 0);

        return *this;
    }

    ~allocate_array_result()
    {
        if (mapping_token)
        {
            auto result = sc::rose_mapping_destroy(mapping_token);
            if (result != sc::result::ok)
            {
                bootinit::log::println(
                    "!!! Warning: failed to free a mapping token: {}.", std::to_underlying(result));
            }
        }

        if (vmo_token)
        {
            auto result = sc::rose_token_release(vmo_token);
            if (result != sc::result::ok)
            {
                bootinit::log::println(
                    "!!! Warning: failed to release a VMO token: {}.", std::to_underlying(result));
            }
        }
    }

    T & operator[](std::ptrdiff_t off) const
    {
        return ptr[off];
    }

    T * ptr;
    std::size_t size;
    std::uintptr_t mapping_token = 0;
    std::uintptr_t vmo_token = 0;
};

template<typename T>
allocate_array_result<T> allocate_array(std::size_t n)
{
    allocate_array_result<T> ret;
    ret.size = n;

    auto result = sc::rose_vmo_create(sizeof(T) * n, 0, &ret.vmo_token);
    if (result != sc::result::ok)
    {
        PANIC("failed to create a VMO!");
    }

    result = sc::rose_mapping_create(self_vas_token, ret.vmo_token, top_of_image, 0, &ret.mapping_token);
    if (result != sc::result::ok)
    {
        PANIC("failed to map a VMO!");
    }

    ret.ptr = reinterpret_cast<T *>(top_of_image);

    top_of_image += sizeof(T) * n;
    top_of_image += kernel::arch::vm::page_sizes[0] - 1;
    top_of_image &= ~(kernel::arch::vm::page_sizes[0] - 1);
    top_of_image += kernel::arch::vm::page_sizes[0];

    return ret;
}

struct create_process_result
{
    std::uintptr_t process_token;
    std::uintptr_t vas_token;
    std::uintptr_t protocol_mailbox_token;
};

struct segment_mapping
{
    allocate_array_result<char> storage;
    std::uintptr_t target_mapping_token;
};

struct loaded_elf
{
    std::string_view filename;
    elf::elf_image image = {};
    allocate_array_result<segment_mapping> segment_mappings = {};
    allocate_array_result<loaded_elf> dependencies = {};

    std::optional<std::uintptr_t> find_symbol_definition(std::string_view name) const
    {
        for (auto && symbol : image.symbols())
        {
            if (symbol.name() == name)
            {
                return reinterpret_cast<std::uintptr_t>(segment_mappings[0].storage.ptr + symbol.value());
            }
        }

        for (std::size_t i = 0; i < dependencies.size; ++i)
        {
            auto maybe = dependencies[i].find_symbol_definition(name);
            if (maybe)
            {
                return maybe;
            }
        }

        return std::nullopt;
    }

    template<typename Visitor>
    void visit_all_relocations(Visitor && vis) const
    {
        for (auto && reloc : image.relocations())
        {
            vis(reloc, *this);
        }

        for (std::size_t i = 0; i < dependencies.size; ++i)
        {
            dependencies[i].visit_all_relocations(vis);
        }
    }
};

loaded_elf load_elf_with_dependencies(
    const archive::cpio & initrd,
    std::string_view filename,
    std::uintptr_t vas_token,
    std::uintptr_t & binary_base)
{
    loaded_elf ret{ filename };

    auto image = [&]
    {
        if (filename == "libvdso.so")
        {
            return std::optional(std::string_view(
                reinterpret_cast<const char *>(bootinit::addresses::vdso.value()), vdso_size));
        }
        return initrd[filename];
    }();
    if (!image)
    {
        PANIC("{} not found in the initrd image!", filename);
    }

    auto prefix = filename == "libvdso.so" ? "kernel" : "initrd";

    bootinit::log::println(" > Attempting to parse {}:{}...", prefix, filename);

    auto elf_result = elf::try_parse(image->data(), image->size());

    if (!elf_result.image)
    {
        PANIC("failed to parse ELF image: {}, offset: {:#018x}", elf_result.error_message, elf_result.offset);
    }

    ret.image = *elf_result.image;
    auto & elf = ret.image;

    bootinit::log::println(" > {}:{}: ELF OS ABI: {}", prefix, filename, elf.get_os_abi_description());
    bootinit::log::println(" > {}:{}: ELF ABI version: {}", prefix, filename, elf.get_abi_version());
    bootinit::log::println(" > {}:{}: ELF type: {}", prefix, filename, elf.get_type_description());
    bootinit::log::println(" > {}:{}: loading at base {:#018x}", prefix, filename, binary_base);

    std::size_t load_segments = 0;
    for (auto && program_header : elf.program_headers())
    {
        if (program_header.type() == elf::segment_types::load)
        {
            ++load_segments;
        }
    }

    ret.segment_mappings = allocate_array<segment_mapping>(load_segments);

    std::size_t i = 0;
    std::size_t final_segment_end;
    for (auto && program_header : elf.program_headers())
    {
        if (program_header.type() != elf::segment_types::load)
        {
            continue;
        }

        auto segment_size = program_header.size();
        auto segment_base = program_header.virtual_address();
        auto segment_end = segment_base + segment_size;

        constexpr auto page_mask = ~(kernel::arch::vm::page_sizes[0] - 1);

        auto segment_end_aligned = (segment_end + kernel::arch::vm::page_sizes[0]) & page_mask;
        final_segment_end = binary_base + segment_end_aligned;

        auto segment_vmo_size = segment_end_aligned - segment_base;

        auto segment_storage = allocate_array<char>(segment_vmo_size);

        auto segment_file_size = program_header.file_size();

        std::memcpy(segment_storage.ptr, image->data() + program_header.offset(), program_header.file_size());
        std::memset(segment_storage.ptr + segment_file_size, 0, segment_size - segment_file_size);

        auto result = sc::rose_mapping_create(
            vas_token,
            segment_storage.vmo_token,
            binary_base + segment_base,
            0,
            &ret.segment_mappings[i].target_mapping_token);
        if (result != sc::result::ok)
        {
            PANIC("failed to map segment #{}'s VMO in the target VAS: {}!", i, std::to_underlying(result));
        }

        ret.segment_mappings.ptr[i++].storage = std::move(segment_storage);
    }

    binary_base = final_segment_end + kernel::arch::vm::page_sizes[0];

    std::size_t dependency_count = 0;
    for (auto && dynamic_entry : elf.dynamic_entries())
    {
        if (dynamic_entry.type() == elf::dynamic_entry_types::needed)
        {
            ++dependency_count;
        }
    }

    if (dependency_count != 0)
    {
        ret.dependencies = allocate_array<loaded_elf>(dependency_count);

        std::size_t i = 0;
        for (auto && dynamic_entry : elf.dynamic_entries())
        {
            if (dynamic_entry.type() == elf::dynamic_entry_types::needed)
            {
                ret.dependencies[i++] =
                    load_elf_with_dependencies(initrd, dynamic_entry.needed_name(), vas_token, binary_base);
            }
        }
    }

    return ret;
}

create_process_result create_process(
    const archive::cpio & initrd,
    std::string_view filename,
    std::string_view name)
{
    create_process_result ret;
    rose::syscall::vdso_mapping_info vdso_info;

    auto result = sc::rose_vas_create(kernel_caps_token, &ret.vas_token, &vdso_info);
    if (result != sc::result::ok)
    {
        PANIC("failed to create a new virtual address space!");
    }

    bootinit::log::println(
        " > {} vdso mapping: {:#018x}-{:#018x}", name, vdso_info.base, vdso_info.base + vdso_info.length);

    std::uintptr_t binary_base = 0x400000;
    auto loaded_elf = load_elf_with_dependencies(initrd, filename, ret.vas_token, binary_base);

    auto reloc_visitor = [&](const elf::relocation & reloc, const struct loaded_elf & elf)
    {
        auto symbol_name = reloc.symbol_name();
        auto symbol_address = loaded_elf.find_symbol_definition(symbol_name);
        if (!symbol_address)
        {
            PANIC("failed to resolve symbol '{}'!", symbol_name);
        }

        std::size_t load_segment_index = 0;
        std::uintptr_t segment_virtual_base = 0;
        bool found = false;
        for (auto && segment : elf.image.program_headers())
        {
            if (segment.type() != elf::segment_types::load)
            {
                continue;
            }

            if (reloc.offset() >= segment.virtual_address()
                && reloc.offset() < segment.virtual_address() + segment.size())
            {
                segment_virtual_base = segment.virtual_address();
                found = true;
                break;
            }

            ++load_segment_index;
        }

        if (!found)
        {
            PANIC("could not find a load segment corresponding to a relocation at {:#018x}", reloc.offset());
        }

        reloc.calculate_and_write(
            *symbol_address, segment_virtual_base, elf.segment_mappings[load_segment_index].storage.ptr);
    };

    loaded_elf.visit_all_relocations(reloc_visitor);

    PANIC("now create a process!");

    return ret;
}
}

[[gnu::section(".bootinit_entry")]] extern "C" int bootinit_main(std::uintptr_t mailbox_token)
{
    __init();

    std::uintptr_t log_read_token;
    std::uintptr_t ack_write_token;

    auto result = sc::rose_mailbox_create(&log_read_token, &bootinit::log::logging_send_mailbox_token);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }
    result = sc::rose_mailbox_create(&bootinit::log::logging_ack_mailbox_token, &ack_write_token);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    sc::mailbox_message message;

    result = sc::rose_mailbox_read(mailbox_token, 0, &message);
    if (result != sc::result::ok || message.type != sc::mailbox_message_type::handle_token)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    auto accept_mailbox_token = message.payload.handle_token;

    message.type = sc::mailbox_message_type::handle_token;
    message.payload.handle_token = log_read_token;
    result = sc::rose_mailbox_write(accept_mailbox_token, &message);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    message.type = sc::mailbox_message_type::handle_token;
    message.payload.handle_token = ack_write_token;
    result = sc::rose_mailbox_write(accept_mailbox_token, &message);
    if (result != sc::result::ok)
    {
        // ... panic ...
        *reinterpret_cast<volatile std::uintptr_t *>(0) = 0;
    }

    bootinit::log::println("Receiving initial handle tokens...");

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok)
    {
        PANIC("Failed to read kernel caps token!");
    }

    if (message.type != sc::mailbox_message_type::handle_token)
    {
        PANIC("Received wrong mailbox message type for kernel caps token!");
    }

    kernel_caps_token = message.payload.handle_token;
    bootinit::log::println(" > Kernel caps token received.");

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok)
    {
        PANIC("Failed to read initrd VMO token!");
    }

    if (message.type != sc::mailbox_message_type::handle_token)
    {
        PANIC("Received wrong mailbox message type for initrd VMO token!");
    }

    [[maybe_unused]] auto initrd_vmo = message.payload.handle_token;
    bootinit::log::println(" > Initrd VMO token received.");

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok)
    {
        PANIC("Failed to read file sizes!");
    }

    if (message.type != sc::mailbox_message_type::user)
    {
        PANIC("Received wrong mailbox message type for file sizes!");
    }

    auto initrd_size = message.payload.user.data0;
    vdso_size = message.payload.user.data1;

    bootinit::log::println(" > Initrd size: {} bytes.", initrd_size);

    result = sc::rose_mailbox_read(mailbox_token, -1, &message);
    if (result != sc::result::ok)
    {
        PANIC("Failed to read bootinit VAS token!");
    }

    if (message.type != sc::mailbox_message_type::handle_token)
    {
        PANIC("Received wrong mailbox message type for bootinit VAS token!");
    }

    self_vas_token = message.payload.handle_token;

    bootinit::log::println("Parsing initrd image...");

    auto initrd_result =
        archive::try_cpio(reinterpret_cast<char *>(bootinit::addresses::initrd.value()), initrd_size);

    if (!initrd_result.archive)
    {
        PANIC(
            "The initrd image is not a valid cpio archive! Reason: {}, header offset: {}.",
            initrd_result.error_message,
            initrd_result.header_offset);
    }

    auto & initrd = *initrd_result.archive;

    bootinit::log::println(" > Number of regular files in initrd: {}.", initrd.size());

    auto test_file = initrd["test-file"];
    if (!test_file)
    {
        PANIC("'test-file not found!");
    }
    bootinit::log::println(" > Contents of test-file: '{}'.", *test_file);

    bootinit::log::println("Creating logger process...");
    [[maybe_unused]] auto [logger_process, logger_vas, logger_protocol_mailbox] =
        create_process(initrd, "system/logger.srv", "logger");

    /*
    bootinit::log::println("Creating vasmgr process...");

    [[maybe_unused]] auto [vasmgr_process, vasmgr_vas, vasmgr_protocol_mailbox] =
        create_process(initrd, "system/vasmgr.srv", "vasmgr");
    */

    /*
    auto procmgr_image = initrd["system/procmgr.srv"];
    if (!procmgr_image)
    {
        PANIC("system/procmgr.srv not found in the initrd image!");
    }

    auto [procmgr_process, procmgr_vas, procmgr_protocol_mailbox] = create_process(*procmgr_image);

    auto broker_image = initrd["system/broker.srv"];
    if (!broker_image)
    {
        PANIC("system/broker.srv not found in the initrd image!");
    }

    auto [broker_process, broker_vas, broker_protocol_mailbox] = create_process(*broker_image);
    */

    for (;;)
        ;
}
