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

#include "generator.h"

#include <iostream>

namespace thorn::generator
{
context::context(context_args args)
    : _scope(std::move(args.scope)),
      _arch(std::move(args.arch)),
      _output_kernel_dir(args.output_dir / "kernel"),
      _output_vdso_dir(args.output_dir / "vdso"),
      _output_user_dir(args.output_dir / "user")
{
    std::error_code ec;

    std::filesystem::remove_all(args.output_dir, ec);
    if (ec)
    {
        std::cerr << "thorn: failed to clear the output directory: " << ec.message() << std::endl;
        std::exit(1);
    }

    std::filesystem::create_directories(_output_kernel_dir, ec);
    if (ec)
    {
        std::cerr << "thorn: failed to create the kernel output directory: " << ec.message() << std::endl;
        std::exit(1);
    }

    std::filesystem::create_directories(_output_vdso_dir, ec);
    if (ec)
    {
        std::cerr << "thorn: failed to create the vdso output directory: " << ec.message() << std::endl;
        std::exit(1);
    }

    std::filesystem::create_directories(_output_user_dir, ec);
    if (ec)
    {
        std::cerr << "thorn: failed to create the user output directory: " << ec.message() << std::endl;
        std::exit(1);
    }
}

context::~context()
{
    for (auto && [_, user_file] : _output_user_files)
    {
        user_file << "}" << std::endl;
    }

    for (auto && [_, vdso_file] : _output_vdso_files)
    {
        vdso_file << "}" << std::endl;
    }

    auto & meta_stream = _output_kernel_files["meta"];

    meta_stream << "using syscall_handler_t = void(*)(context &);\n";
    meta_stream << "const std::size_t syscall_count = " << _syscalls.size() << ";\n";
    meta_stream << "syscall_handler_t syscall_table[syscall_count] = {\n";

    for (auto && [_, syscall] : _syscalls)
    {
        meta_stream << "    &handle_syscall_" << syscall << ",\n";
    }

    meta_stream << "};\n\n";

    for (auto && [_, kernel_file] : _output_kernel_files)
    {
        kernel_file << "}" << std::endl;
    }

    std::fstream meta_header(_output_kernel_dir / "meta.h", std::ios::out | std::ios::trunc);
    meta_header << "/** THIS IS A GENERATED FILE, DO NOT MODIFY MANUALLY **/\n";
    meta_header << "\n";

    meta_header << "namespace kernel::" << _arch << "::syscalls\n";
    meta_header << "{\n";
    meta_header << "using syscall_handler_t = void(*)(context &);\n";
    meta_header << "extern const std::size_t syscall_count = " << _syscalls.size() << ";\n";
    meta_header << "extern syscall_handler_t syscall_table[syscall_count];\n";
    meta_header << "}\n";
}

void context::set_dependencies(const std::unordered_map<std::string, std::unordered_set<std::string>> & deps)
{
    _dependencies = deps;
}

void context::set_includes(const std::unordered_map<std::string, std::unordered_set<std::string>> & includes)
{
    _includes = includes;
}

void context::generate_symbol(std::string_view name, const symbol & symb)
{
    auto & user_stream = _get_stream(symb.module, _stream_kind::user);
    auto & vdso_stream = _get_stream(symb.module, _stream_kind::vdso);
    auto & kernel_stream = _get_stream(symb.module, _stream_kind::kernel);

    switch (symb.description.index())
    {
        case 0:
            std::cerr << "thorn: internal error: empty symbol description variant for symbol '" << name
                      << "' from module '" << symb.module << "'" << std::endl;
            std::exit(1);

        case 1:
        {
            auto && enum_ = std::get<1>(symb.description);

            user_stream << "enum class " << name << " : std::uintptr_t\n";
            user_stream << "{\n";
            for (auto && [name, value] : enum_.enumerations)
            {
                user_stream << "    " << name << " = " << value << ",\n";
            }
            user_stream << "};\n" << std::endl;

            break;
        }

        case 2:
        {
            auto && struct_ = std::get<2>(symb.description);

            user_stream << "struct " << name << '\n';
            user_stream << "{\n";
            for (auto && [name, type] : struct_.members)
            {
                switch (type.index())
                {
                    case 0:
                    {
                        auto && type_name = std::get<0>(type);
                        user_stream << "    ::" << type_name << " " << name << ";\n";
                        break;
                    }

                    case 1:
                    {
                        auto && union_ = std::get<1>(type);

                        user_stream << "    union {\n";
                        for (auto && [uname, utype] : union_.members)
                        {
                            user_stream << "        ::" << utype << " " << uname << ";\n";
                        }
                        user_stream << "    } " << name << ";\n";

                        break;
                    }
                }
            }
            user_stream << "};\n" << std::endl;

            break;
        }

        case 3:
        {
            auto && syscall = std::get<3>(symb.description);

            _syscalls[syscall.id] = name;

            auto return_type = (syscall.return_type ? *syscall.return_type : "void");

            user_stream << "extern \"C\" " << return_type << " " << name << "(\n";

            vdso_stream << return_type << " " << name << "(\n";

            kernel_stream << "void handle_syscall_" << name << "(context & ctx)\n";
            kernel_stream << "{\n";
            kernel_stream << "    auto current_thread = cpu::get_core_local_storage()->current_thread;\n\n";

            if (symb.module != "meta")
            {
                auto & meta_kernel_stream = _get_stream(symb.module, _stream_kind::kernel);

                meta_kernel_stream << "void handle_syscall_" << name << "(context & ctx);\n";
            }

            bool first_param = true;
            for (auto && [pname, param] : syscall.parameters)
            {
                if (!first_param)
                {
                    user_stream << ",\n";
                    vdso_stream << ",\n";
                }
                else
                {
                    first_param = false;
                }

                switch (param.index())
                {
                    case 0:
                    {
                        auto && type_name = std::get<0>(param);

                        user_stream << "    ::" << type_name << " " << pname;
                        vdso_stream << "    ::" << type_name;

                        break;
                    }

                    case 1:
                    {
                        auto && handle_desc = std::get<1>(param);

                        user_stream << "    ::std::uintptr_t " << pname;
                        vdso_stream << "    ::std::uintptr_t";

                        break;
                    }

                    case 2:
                    {
                        auto && pointer_desc = std::get<2>(param);

                        user_stream << "    " << (!pointer_desc.out ? "const " : "")
                                    << "::" << pointer_desc.deref_type << " * " << pname;

                        vdso_stream << "    " << (!pointer_desc.out ? "const " : "")
                                    << "::" << pointer_desc.deref_type << " *";
                        break;
                    }
                }
            }

            user_stream << "\n);\n" << std::endl;

            vdso_stream << "\n)\n{\n";
            if (syscall.return_type)
            {
                vdso_stream << "    ::" << *syscall.return_type << " ret;\n";
            }
            vdso_stream << "    asm volatile(R\"(\n";
            vdso_stream << "        mov $" << syscall.id << ", %%rax\n";

            auto && arch_translation = arch_arg_register_translation.at(_arch);
            for (std::size_t i = arch_registers_for_args.at(_arch).size() - syscall.parameters.size();
                 i < arch_translation.size();
                 ++i)
            {
                vdso_stream << "        mov %%" << arch_translation[i].first << ", %%"
                            << arch_translation[i].second << "\n";
            }

            vdso_stream << "        .globl __rose_syscall_" << syscall.id << "\n";
            vdso_stream << "        __rose_syscall_" << syscall.id << ":\n";
            vdso_stream << "        syscall\n";
            vdso_stream << "    )\" : \"=a\"(ret) :: \"r11\", \"memory\");\n";
            if (syscall.return_type)
            {
                vdso_stream << "    return ret;\n";
            }
            vdso_stream << "}\n" << std::endl;

            std::size_t i = -1;
            for (auto && [pname, param] : syscall.parameters)
            {
                ++i;

                switch (param.index())
                {
                    case 0:
                    {
                        kernel_stream << "    " << std::get<0>(param) << " " << pname << " = ctx."
                                      << _get_arch_register_for_arg(i) << ";\n\n";

                        break;
                    }

                    case 1:
                    {
                        auto && handle_spec = std::get<1>(param);

                        kernel_stream << "    handle_token_t " << pname << "_token{ctx."
                                      << _get_arch_register_for_arg(i) << "};\n";
                        kernel_stream << "    auto " << pname
                                      << "_handle = current_thread->get_container()->get_handle(" << pname
                                      << "_token);\n\n";

                        kernel_stream << "    if (!" << pname << "_handle)\n";
                        kernel_stream << "    {\n";
                        kernel_stream << "        ctx.rax = std::to_underlying(::" << _scope
                                      << "::result::invalid_token);\n";
                        kernel_stream << "        return;\n";
                        kernel_stream << "    }\n\n";

                        kernel_stream << "    if (!" << pname << "_handle->is_a<" << handle_spec.deref_type
                                      << ">())\n";
                        kernel_stream << "    {\n";
                        kernel_stream << "        ctx.rax = std::to_underlying(::" << _scope
                                      << "::result::wrong_handle_type);\n";
                        kernel_stream << "        return;\n";
                        kernel_stream << "    }\n\n";

                        // TODO: fold into a single has_permissions call
                        for (auto && perm : handle_spec.required_permissions)
                        {
                            kernel_stream << "    if (!" << pname
                                          << "_handle->has_permissions(kernel::permissions::" << perm
                                          << "))\n";
                            kernel_stream << "    {\n";
                            kernel_stream << "        ctx.rax = std::to_underlying(::" << _scope
                                          << "::result::not_allowed);\n";
                            kernel_stream << "        return;\n";
                            kernel_stream << "    }\n\n";
                        }

                        kernel_stream << "    auto " << pname << " = " << pname << "_handle->get_as<"
                                      << handle_spec.deref_type << ">();\n\n";

                        break;
                    }

                    case 2:
                    {
                        auto && pointer_spec = std::get<2>(param);

                        kernel_stream << "    auto " << pname << "_addr = reinterpret_cast<"
                                      << pointer_spec.deref_type << " *>(ctx."
                                      << _get_arch_register_for_arg(i) << ");\n\n";

                        kernel_stream
                            << "    auto " << pname
                            << "_guard = current_thread->get_container()->get_vas()->lock_array_mapping("
                            << pname << "_addr, 1, " << (pointer_spec.out ? "true" : "false") << ");\n";
                        kernel_stream << "    if (!" << pname << "_guard)\n";
                        kernel_stream << "    {\n";
                        kernel_stream << "        ctx.rax = std::to_underlying(::" << _scope
                                      << "::result::invalid_pointers);\n";
                        kernel_stream << "        return;\n";
                        kernel_stream << "    }\n\n";

                        kernel_stream << "    " << pointer_spec.deref_type << " " << pname << "_storage;\n";
                        kernel_stream << "    auto " << pname << " = &" << pname << "_storage;\n\n";

                        if (pointer_spec.in)
                        {
                            kernel_stream << "    std::memcpy(&" << pname << "_storage, " << pname
                                          << "_addr, sizeof(" << pointer_spec.deref_type << "));\n\n";
                        }

                        break;
                    }
                }
            }

            kernel_stream << "    auto result = " << syscall.implementation_scope << "::syscall_" << name
                          << "_handler(\n";
            for (std::size_t i = 0; i < syscall.parameters.size(); ++i)
            {
                kernel_stream << "        " << syscall.parameters[i].first
                              << (i == syscall.parameters.size() - 1 ? "" : ",") << "\n";
            }
            kernel_stream << "    );\n\n";

            kernel_stream << "    ctx.rax = std::to_underlying(result);\n";
            kernel_stream << "    if (result != ::" << _scope << "::result::ok)\n";
            kernel_stream << "    {\n";
            kernel_stream << "        return;\n";
            kernel_stream << "    }\n\n";

            for (auto && [pname, param] : syscall.parameters)
            {
                if (param.index() == 2)
                {
                    auto && pointer_desc = std::get<2>(param);

                    if (!pointer_desc.out)
                    {
                        continue;
                    }

                    kernel_stream << "    std::memcpy(" << pname << "_addr, &" << pname << "_storage, sizeof("
                                  << pointer_desc.deref_type << "));\n\n";
                }
            }

            kernel_stream << "}\n" << std::endl;
        }
    }
}

std::fstream & context::_get_stream(std::string module, _stream_kind kind)
{
    auto & output_files = _get_stream_map(kind);

    auto it = output_files.find(module);
    if (it != output_files.end())
    {
        return it->second;
    }

    std::fstream stream(
        (_get_target_dir(kind) / module).replace_extension(_get_extension(kind)),
        std::ios::out | std::ios::trunc);
    stream << "/** THIS IS A GENERATED FILE, DO NOT MODIFY MANUALLY **/\n";
    stream << "\n";

    switch (kind)
    {
        case _stream_kind::user:
            stream << "#pragma once\n";
            stream << "\n";
            stream << "#include <cstdint>\n";
            break;

        case _stream_kind::vdso:
        case _stream_kind::kernel:
            stream << "#include \"../user/"
                   << std::filesystem::path(module)
                          .replace_extension(_get_extension(_stream_kind::user))
                          .string()
                   << "\"\n";
            break;
    }

    stream << "\n";
    if (kind == _stream_kind::kernel)
    {
        stream << "#include <arch/" << _arch << "/cpu/core.h>\n";
        stream << "#include <arch/" << _arch << "/cpu/syscalls.h>\n";
        stream << "#include <arch/" << _arch << "/memory/vm.h>\n";
        stream << "#include <util/handle.h>\n";
        stream << "#include <scheduler/thread.h>\n";
        stream << "\n";

        auto it = _includes.find(module);
        if (it != _includes.end())
        {
            for (auto && header : it->second)
            {
                stream << "#include <" << header << ">\n";
            }
        }
        stream << "\n";

        stream << "namespace kernel::" << _arch << "::syscalls\n";
    }
    else
    {
        if (kind == _stream_kind::user)
        {
            auto it = _dependencies.find(module);
            if (it != _dependencies.end())
            {
                for (auto && header : it->second)
                {
                    stream << "#include \"" << header << "\"\n";
                }
            }
            stream << "\n";
        }

        stream << "namespace " << _scope << "\n";
    }
    stream << "{\n" << std::endl;

    auto [new_it, _] = output_files.insert(std::make_pair(std::move(module), std::move(stream)));
    return new_it->second;
}
}
