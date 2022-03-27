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

#include "generator.h"
#include "parser.h"

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace thorn
{
struct context_args
{
    std::string scope;
    std::string arch;
    std::filesystem::path input_dir;
    std::filesystem::path output_dir;
};

class context
{
public:
    context(context_args args) : _args(std::move(args))
    {
    }

    void process(std::string filename);
    void output();

private:
    std::optional<std::string> _process_qualified_name(
        std::string_view module,
        const parser::qualified_name & ast);

    void _insert_enum(std::string_view module, const parser::parsed_enum & ast);
    void _insert_struct(std::string_view module, const parser::parsed_struct & ast);
    void _insert_syscall(std::string_view module, const parser::parsed_syscall & ast);
    void _insert_include(std::string_view module, const parser::parsed_include & ast);

    void _insert_symbol(const std::string & name, generator::symbol symb);

    context_args _args;

    std::unordered_map<std::string, generator::symbol> _symbols;
    std::unordered_map<std::string, std::unordered_set<std::string>> _module_deps;
    std::unordered_map<std::string, std::unordered_set<std::string>> _module_includes;
    std::vector<std::string> _syscalls;
    std::vector<std::string> _symbols_in_order;
    std::unordered_map<std::string, std::vector<std::string>> _permissions;
    std::optional<std::vector<std::string>> _global_permissions;
};
}
