/*
 * Copyright © 2022-2025 Michał 'Griwes' Dominiak
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

#include "process.h"

#include "../arch/vm.h"
#include "addresses.h"
#include "print.h"

#include <user/meta.h>

#include <algorithm>
#include <new>

namespace sc = rose::syscall;

namespace bootinit::facts
{
std::uintptr_t acceptor_mailbox_token;
std::uintptr_t kernel_caps_token;
std::uintptr_t self_vas_token;
std::uintptr_t vdso_size;
}

namespace bootinit::process
{
std::uintptr_t top_of_image = addresses::top_of_stack.value() + kernel::arch::vm::page_sizes[0];

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
                kernel_print::println(
                    "!!! Warning: failed to free a mapping token: {}.", std::to_underlying(result));
            }
        }

        if (vmo_token)
        {
            auto result = sc::rose_token_release(vmo_token);
            if (result != sc::result::ok)
            {
                kernel_print::println(
                    "!!! Warning: failed to release a VMO token: {}.", std::to_underlying(result));
            }
        }
    }

    T & operator[](std::ptrdiff_t off) const
    {
        return ptr[off];
    }

    auto begin() const
    {
        return ptr;
    }

    auto end() const
    {
        return ptr + size;
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

    result =
        sc::rose_mapping_create(facts::self_vas_token, ret.vmo_token, top_of_image, 0, &ret.mapping_token);
    if (result != sc::result::ok)
    {
        PANIC("failed to map a VMO!");
    }

    ret.ptr = new (reinterpret_cast<T *>(top_of_image)) T[n];

    top_of_image += sizeof(T) * n;
    top_of_image += kernel::arch::vm::page_sizes[0] - 1;
    top_of_image &= ~(kernel::arch::vm::page_sizes[0] - 1);
    top_of_image += kernel::arch::vm::page_sizes[0];

    return ret;
}

struct segment_mapping
{
    allocate_array_result<char> storage;
    std::uintptr_t target_mapping_token;
};

struct loaded_elf
{
    std::string_view filename;
    std::optional<std::string_view> image = std::nullopt;
    elf::elf_image elf = {};
    allocate_array_result<segment_mapping> segment_mappings = {};
    allocate_array_result<std::variant<loaded_elf, const loaded_elf *>> dependencies = {};
    std::uintptr_t elf_base = 0;
    allocate_array_result<const loaded_elf *> init_order = {};

    auto all_dependencies() const
    {
        struct all_dependencies_view
        {
            struct iterator
            {
                friend struct all_dependencies_view;

                iterator & operator++()
                {
                    ++_index;
                    return *this;
                }

                const loaded_elf & operator*() const
                {
                    auto & v = _self->dependencies[_index];
                    switch (v.index())
                    {
                        case 0:
                            return std::get<0>(v);
                        case 1:
                            return *std::get<1>(v);
                        default:
                            PANIC("a dependency variant had an unknown index");
                    }
                }

                bool operator==(const iterator &) const = default;

            private:
                const loaded_elf * _self;
                std::size_t _index;
            };

            all_dependencies_view(const loaded_elf * self) : _self(self)
            {
            }

            iterator begin() const
            {
                iterator ret;
                ret._self = _self;
                ret._index = 0;
                return ret;
            }

            iterator end() const
            {
                iterator ret;
                ret._self = _self;
                ret._index = _self->dependencies.size;
                return ret;
            }

        private:
            const loaded_elf * _self;
        };

        return all_dependencies_view(this);
    }

    auto owned_dependencies() const
    {
        struct owned_dependencies_view
        {
            struct iterator
            {
                friend struct owned_dependencies_view;

                iterator & operator++()
                {
                    do
                    {
                        ++_index;
                    } while (_index < _self->dependencies.size
                             && std::holds_alternative<const loaded_elf *>(_self->dependencies[_index]));

                    return *this;
                }

                loaded_elf & operator*() const
                {
                    return std::get<0>(_self->dependencies[_index]);
                }

                bool operator==(const iterator &) const = default;

            private:
                const loaded_elf * _self;
                std::size_t _index;
            };

            owned_dependencies_view(const loaded_elf * self) : _self(self)
            {
            }

            iterator begin() const
            {
                iterator ret;
                ret._self = _self;
                ret._index = 0;
                return ret;
            }

            iterator end() const
            {
                iterator ret;
                ret._self = _self;
                ret._index = _self->dependencies.size;
                return ret;
            }

        private:
            const loaded_elf * _self;
        };

        return owned_dependencies_view(this);
    }

    const loaded_elf * is_shared_object_prepared(std::string_view name) const
    {
        if (filename == name)
        {
            return this;
        }

        for (auto && dependency : dependencies)
        {
            if (auto found = std::get<loaded_elf>(dependency).is_shared_object_prepared(name))
            {
                return found;
            }
        }

        return nullptr;
    }

    std::size_t depth() const
    {
        std::size_t ret = 0;
        for (auto && dependency : owned_dependencies())
        {
            auto dep_depth = dependency.depth();
            if (dep_depth > ret)
            {
                ret = dep_depth;
            }
        }
        return ret + 1;
    }

    std::size_t count() const
    {
        std::size_t ret = 1;
        for (auto && dependency : owned_dependencies())
        {
            ret += dependency.count();
        }
        return ret;
    }

    void compute_init_order_at_depth(
        std::size_t depth,
        allocate_array_result<const loaded_elf *> & init_order_array,
        std::size_t & init_order_remaining)
    {
        if (depth == 0)
        {
            for (auto && dependency : all_dependencies())
            {
                auto dep_ptr = &dependency;
                auto it = std::find(
                    init_order_array.begin() + init_order_remaining, init_order_array.end(), dep_ptr);
                if (it == init_order_array.end())
                {
                    init_order_array[--init_order_remaining] = dep_ptr;
                    continue;
                }

                std::rotate(init_order_array.begin() + init_order_remaining, it, it + 1);
            }

            return;
        }

        for (auto && dependency : owned_dependencies())
        {
            dependency.compute_init_order_at_depth(depth - 1, init_order_array, init_order_remaining);
        }
    }

    void compute_init_order()
    {
        if (init_order.ptr)
        {
            return;
        }

        return compute_init_order(depth());
    }

    void compute_init_order(std::size_t depth)
    {
        if (init_order.ptr)
        {
            return;
        }

        auto image_count = count();
        init_order = allocate_array<const loaded_elf *>(image_count);
        std::size_t init_order_remaining = image_count;

        init_order[--init_order_remaining] = this;

        for (std::size_t i = 0; i < depth; ++i)
        {
            compute_init_order_at_depth(i, init_order, init_order_remaining);
        }

        if (init_order_remaining)
        {
            PANIC("load_all did not fill the entire init order array!");
        }

        kernel_print::println(" > Init order for initrd:{}:", filename);
        for (std::size_t i = 0; i < image_count; ++i)
        {
            kernel_print::println(" >> {}", init_order[i]->filename);
        }
    }

#define DEFINE_COUNT_MEMBER(x)                                                                               \
    std::size_t x##_count() const                                                                            \
    {                                                                                                        \
        std::size_t ret = elf.get_##x() != std::nullopt;                                                     \
        for (auto && dependency : owned_dependencies())                                                      \
        {                                                                                                    \
            ret += dependency.x##_count();                                                                   \
        }                                                                                                    \
        return ret;                                                                                          \
    }

    DEFINE_COUNT_MEMBER(preinit);
    DEFINE_COUNT_MEMBER(init);
    DEFINE_COUNT_MEMBER(fini);

#undef DEFINE_COUNT_MEMBER

    template<typename Visitor>
    void visit_all_preinits(Visitor && vis) const
    {
        for (auto * elf : init_order)
        {
            auto preinit = elf->elf.get_preinit();
            if (preinit)
            {
                vis(elf->elf_base + preinit->offset, elf->elf_base + preinit->offset + preinit->byte_size);
            }
        }
    }

    template<typename Visitor>
    void visit_all_inits(Visitor && vis) const
    {
        for (auto * elf : init_order)
        {
            auto init = elf->elf.get_init();
            if (init)
            {
                vis(elf->elf_base + init->offset, elf->elf_base + init->offset + init->byte_size);
            }
        }
    }

    template<typename Visitor>
    void visit_all_finis(Visitor && vis) const
    {
        for (auto it = init_order.end(); it != init_order.begin(); --it)
        {
            auto * elf = *(it - 1);
            auto fini = elf->elf.get_fini();
            if (fini)
            {
                vis(elf->elf_base + fini->offset, elf->elf_base + fini->offset + fini->byte_size);
            }
        }
    }

    std::optional<std::uintptr_t> find_symbol_definition_at_depth(std::string_view name, std::size_t depth)
        const
    {
        if (depth == 0)
        {
            for (auto && symbol : elf.symbols())
            {
                if (symbol.name() == name && symbol.is_defined())
                {
                    return reinterpret_cast<std::uintptr_t>(elf_base + symbol.value());
                }
            }

            return std::nullopt;
        }

        for (auto && dependency : all_dependencies())
        {
            auto maybe = dependency.find_symbol_definition_at_depth(name, depth - 1);
            if (maybe)
            {
                return maybe;
            }
        }

        return std::nullopt;
    }

    std::optional<std::uintptr_t> find_symbol_definition(std::string_view name, std::size_t depth) const
    {
        for (std::size_t i = 0; i < depth; ++i)
        {
            auto maybe = find_symbol_definition_at_depth(name, i);
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
        for (auto && reloc : elf.relocations())
        {
            vis(reloc, *this);
        }

        for (auto && dependency : owned_dependencies())
        {
            dependency.visit_all_relocations(vis);
        }
    }
};

loaded_elf load_elf_with_dependencies(
    const archive::cpio & initrd,
    std::string_view filename,
    std::uintptr_t vdso_preload,
    std::uintptr_t vas_token,
    std::uintptr_t & binary_base,
    const loaded_elf * root = nullptr)
{
    loaded_elf ret{ filename };
    if (!root)
    {
        root = &ret;
    }

    ret.image = [&]
    {
        if (filename == "libvdso.so")
        {
            return std::optional(
                std::string_view(reinterpret_cast<const char *>(addresses::vdso.value()), facts::vdso_size));
        }
        return initrd[filename];
    }();
    if (!ret.image)
    {
        PANIC("{} not found in the initrd image!", filename);
    }

    auto prefix = filename == "libvdso.so" ? "kernel" : "initrd";

    kernel_print::println(" > Attempting to parse {}:{}...", prefix, filename);

    auto elf_result = elf::try_parse(ret.image->data(), ret.image->size());

    if (!elf_result.image)
    {
        PANIC("failed to parse ELF image: {}, offset: {:#018x}", elf_result.error_message, elf_result.offset);
    }

    ret.elf = *elf_result.image;

    kernel_print::println(" >> {}:{}: ELF OS ABI: {}", prefix, filename, ret.elf.get_os_abi_description());
    kernel_print::println(" >> {}:{}: ELF ABI version: {}", prefix, filename, ret.elf.get_abi_version());
    kernel_print::println(" >> {}:{}: ELF type: {}", prefix, filename, ret.elf.get_type_description());

    if (filename == "libvdso.so")
    {
        kernel_print::println(" >> kernel:libvdso.so: preloaded at base {:#018x}", vdso_preload);
        ret.elf_base = vdso_preload;
        return ret;
    }

    kernel_print::println(" >> initrd:{}: loading at base {:#018x}", filename, binary_base);
    ret.elf_base = binary_base;

    std::size_t load_segments = 0;
    for (auto && program_header : ret.elf.program_headers())
    {
        if (program_header.type() == elf::segment_types::load)
        {
            ++load_segments;
        }
    }

    ret.segment_mappings = allocate_array<segment_mapping>(load_segments);

    std::size_t i = 0;
    std::size_t final_segment_end;
    for (auto && program_header : ret.elf.program_headers())
    {
        if (program_header.type() != elf::segment_types::load)
        {
            continue;
        }

        auto segment_size = program_header.size();
        auto segment_base = program_header.virtual_address();
        auto segment_end = segment_base + segment_size;

        constexpr auto page_mask = ~(kernel::arch::vm::page_sizes[0] - 1);

        auto segment_end_aligned = (segment_end + kernel::arch::vm::page_sizes[0] - 1) & page_mask;
        final_segment_end = binary_base + segment_end_aligned;

        auto segment_vmo_size = segment_end_aligned - segment_base;

        auto segment_storage = allocate_array<char>(segment_vmo_size);

        auto segment_file_size = program_header.file_size();

        std::memcpy(
            segment_storage.ptr, ret.image->data() + program_header.offset(), program_header.file_size());
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
    for (auto && dynamic_entry : ret.elf.dynamic_entries())
    {
        if (dynamic_entry.type() == elf::dynamic_entry_types::needed)
        {
            ++dependency_count;
        }
    }

    if (dependency_count != 0)
    {
        ret.dependencies = allocate_array<std::variant<loaded_elf, const loaded_elf *>>(dependency_count);

        std::size_t i = 0;
        for (auto && dynamic_entry : ret.elf.dynamic_entries())
        {
            if (dynamic_entry.type() == elf::dynamic_entry_types::needed)
            {
                auto name = dynamic_entry.needed_name();
                if (auto found = root->is_shared_object_prepared(name))
                {
                    ret.dependencies[i] = found;
                }
                else
                {
                    kernel_print::println(" >> {}:{}: Found dependency: {}.", prefix, filename, name);

                    ret.dependencies[i] =
                        load_elf_with_dependencies(initrd, name, vdso_preload, vas_token, binary_base, root);
                }

                ++i;
            }
        }
    }

    return ret;
}

void send_runtime_initialization(loaded_elf & image, std::uintptr_t protocol_mailbox)
{
    image.compute_init_order();

    std::uintptr_t runtime_mailbox_read, runtime_mailbox;
    auto result = sc::rose_mailbox_create(&runtime_mailbox_read, &runtime_mailbox);
    if (result != sc::result::ok)
    {
        PANIC("failed to create a runtime init mailbox!");
    }

    auto visitor = [&](std::uintptr_t begin, std::uintptr_t end)
    {
        sc::mailbox_message message;
        message.type = sc::mailbox_message_type::user;
        message.payload.user = { .data0 = begin, .data1 = end };

        auto result = sc::rose_mailbox_write(runtime_mailbox, &message);
        if (result != sc::result::ok)
        {
            PANIC("failed to send a message on the runtime init mailbox!");
        }

        kernel_print::println(" >> Sent ctor/dtor data: {:#018x}-{:#018x}.", begin, end);
    };

    sc::mailbox_message message;

    auto init_count = image.preinit_count() + image.init_count();

    message.type = sc::mailbox_message_type::user;
    message.payload.user = { .data0 = init_count, .data1 = 0 };

    result = sc::rose_mailbox_write(runtime_mailbox, &message);
    if (result != sc::result::ok)
    {
        PANIC("failed to send a message on the runtime init mailbox!");
    }

    kernel_print::println(" > Sending preinit arrays...");
    image.visit_all_preinits(visitor);
    kernel_print::println(" > Sending init arrays...");
    image.visit_all_inits(visitor);

    auto fini_count = image.fini_count();

    message.type = sc::mailbox_message_type::user;
    message.payload.user = { .data0 = fini_count, .data1 = 0 };

    result = sc::rose_mailbox_write(runtime_mailbox, &message);
    if (result != sc::result::ok)
    {
        PANIC("failed to send a message on the runtime init mailbox!");
    }

    kernel_print::println(" > Sending fini arrays...");
    image.visit_all_finis(visitor);

    message.type = sc::mailbox_message_type::handle_token;
    message.payload.handle_token = runtime_mailbox_read;

    result = sc::rose_mailbox_write(protocol_mailbox, &message);
    if (result != sc::result::ok)
    {
        PANIC("failed to send a handle to the runtime init mailbox!");
    }

    result = sc::rose_token_release(runtime_mailbox);
    if (result != sc::result::ok)
    {
        kernel_print::println("!!! Warning: failed to release a token: {}.", std::to_underlying(result));
    }
}

create_process_result create_process(
    const archive::cpio & initrd,
    std::string_view filename,
    std::string_view name)
{
    create_process_result ret;
    rose::syscall::vdso_mapping_info vdso_info;

    auto result = sc::rose_vas_create(facts::kernel_caps_token, &ret.vas_token, &vdso_info);
    if (result != sc::result::ok)
    {
        PANIC("failed to create a new virtual address space!");
    }

    kernel_print::println(
        " > {} vdso mapping: {:#018x}-{:#018x}", name, vdso_info.base, vdso_info.base + vdso_info.length);

    std::uintptr_t binary_base_c = 0x400000;
    std::uintptr_t binary_base = binary_base_c;
    auto loaded_elf =
        load_elf_with_dependencies(initrd, filename, vdso_info.base, ret.vas_token, binary_base);

    auto depth = loaded_elf.depth();

    auto reloc_visitor = [&](const elf::relocation & reloc, const struct loaded_elf & elf)
    {
        std::uintptr_t symbol_value = 0;
        if (reloc.needs_symbol())
        {
            auto symbol_name = reloc.symbol_name();
            auto symbol_address = loaded_elf.find_symbol_definition(symbol_name, depth);
            if (!symbol_address)
            {
                PANIC("failed to resolve symbol '{}' in {}!", symbol_name, elf.filename);
            }
            symbol_value = *symbol_address;
        }

        std::size_t load_segment_index = 0;
        std::uintptr_t segment_virtual_base = 0;
        bool found = false;
        for (auto && segment : elf.elf.program_headers())
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

        auto [target_address, reloc_value] = reloc.calculate_and_write(
            symbol_value,
            elf.elf_base,
            segment_virtual_base,
            elf.segment_mappings[load_segment_index].storage.ptr);

        kernel_print::println(
            " >> Applied relocation, address: {:#018x}, value: {:#x}.", target_address, reloc_value);
    };

    kernel_print::println(" > Applying relocations...");
    loaded_elf.visit_all_relocations(reloc_visitor);

    result = sc::rose_process_create(facts::kernel_caps_token, ret.vas_token, &ret.process_token);
    if (result != sc::result::ok)
    {
        PANIC("failed to create a process!");
    }

    std::uintptr_t stack_token;
    result = sc::rose_vmo_create(31 * kernel::arch::vm::page_sizes[0], 0, &stack_token);
    if (result != sc::result::ok)
    {
        PANIC("failed to create a stack VMO!");
    }

    std::uintptr_t top_of_stack = 0x80000000;
    std::uintptr_t stack_map_base = top_of_stack - 31 * kernel::arch::vm::page_sizes[0];

    std::uintptr_t stack_mapping;
    result = sc::rose_mapping_create(ret.vas_token, stack_token, stack_map_base, 0, &stack_mapping);
    if (result != sc::result::ok)
    {
        PANIC("failed to map a stack VMO!");
    }

    result = sc::rose_token_release(stack_token);
    if (result != sc::result::ok)
    {
        kernel_print::println("!!! Warning: failed to release a token: {}.", std::to_underlying(result));
    }

    result = sc::rose_token_release(stack_mapping);
    if (result != sc::result::ok)
    {
        kernel_print::println("!!! Warning: failed to release a token: {}.", std::to_underlying(result));
    }

    std::uintptr_t read_endpoint;
    result = sc::rose_mailbox_create(&read_endpoint, &ret.protocol_mailbox_token);
    if (result != sc::result::ok)
    {
        PANIC("failed to create a protocol mailbox!");
    }

    send_runtime_initialization(loaded_elf, ret.protocol_mailbox_token);

    result = sc::rose_process_start(
        ret.process_token, binary_base_c + loaded_elf.elf.get_entry_point(), top_of_stack, read_endpoint);
    if (result != sc::result::ok)
    {
        PANIC("failed to start the process!");
    }

    return ret;
}
}
