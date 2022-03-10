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

#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace thorn::generator
{
struct enum_description
{
    std::vector<std::pair<std::string, std::uintptr_t>> enumerations;
};

struct union_description
{
    std::vector<std::pair<std::string, std::string>> members;
};

struct struct_description
{
    std::vector<std::pair<std::string, std::variant<std::string, union_description>>> members;
};

struct handle_token
{
    std::vector<std::string> required_permissions;
    std::string deref_type;
};

struct pointer
{
    bool in = false;
    bool out = false;
    std::string deref_type;
};

struct syscall_description
{
    std::string implementation_scope;
    std::vector<std::pair<std::string, std::variant<std::string, handle_token, pointer>>> parameters;
    std::optional<std::string> return_type;
    std::size_t id;
    bool blocking = false;
};

struct symbol
{
    std::string module;
    std::variant<std::monostate, enum_description, struct_description, syscall_description> description;
};

struct context_args
{
    std::string scope;
    std::string arch;
    std::filesystem::path output_dir;
};

inline const std::unordered_map<std::string, std::vector<std::string>> arch_registers_for_args = {
    { "amd64", { "rdi", "rsi", "r8", "r9", "r10", "r12" } }, //, "r13", "r14", "r15" } }
};

inline const std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>
    arch_arg_register_translation = {
        { "amd64", { { "r9", "r12" }, { "r8", "r10" }, { "rcx", "r9" }, { "rdx", "r8" } } }
    };

class context
{
public:
    context(context_args args);
    ~context();

    void set_dependencies(const std::unordered_map<std::string, std::unordered_set<std::string>> & deps);
    void set_includes(const std::unordered_map<std::string, std::unordered_set<std::string>> & deps);
    void generate_symbol(std::string_view name, const symbol & symb);

private:
    enum class _stream_kind
    {
        kernel,
        vdso,
        user
    };

    std::fstream & _get_stream(std::string module, _stream_kind);

    auto & _get_stream_map(_stream_kind kind)
    {
        switch (kind)
        {
            case _stream_kind::kernel:
                return _output_kernel_files;
            case _stream_kind::vdso:
                return _output_vdso_files;
            case _stream_kind::user:
                return _output_user_files;
        }

        __builtin_unreachable();
    }

    auto _get_extension(_stream_kind kind)
    {
        switch (kind)
        {
            case _stream_kind::kernel:
                [[fallthrough]];
            case _stream_kind::vdso:
                return ".cpp";
            case _stream_kind::user:
                return ".h";
        }

        __builtin_unreachable();
    }

    auto & _get_target_dir(_stream_kind kind)
    {
        switch (kind)
        {
            case _stream_kind::kernel:
                return _output_kernel_dir;
            case _stream_kind::vdso:
                return _output_vdso_dir;
            case _stream_kind::user:
                return _output_user_dir;
        }

        __builtin_unreachable();
    }

    auto _get_arch_register_for_arg(std::size_t i)
    {
        return arch_registers_for_args.at(_arch).at(i);
    }

    std::string _scope;
    std::string _arch;

    std::unordered_map<std::string, std::unordered_set<std::string>> _dependencies;
    std::unordered_map<std::string, std::unordered_set<std::string>> _includes;

    std::filesystem::path _output_kernel_dir;
    std::filesystem::path _output_vdso_dir;
    std::filesystem::path _output_user_dir;

    std::unordered_map<std::string, std::fstream> _output_kernel_files;
    std::unordered_map<std::string, std::fstream> _output_vdso_files;
    std::unordered_map<std::string, std::fstream> _output_user_files;
    std::fstream _output_kernel_header;

    std::map<std::size_t, std::string> _syscalls;
};
}
