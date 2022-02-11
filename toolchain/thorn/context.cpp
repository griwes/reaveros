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

#include "context.h"

#include <random>

namespace thorn
{
void context::process(std::string filename)
{
    auto it = lexer::tokenize(_args.input_dir / (filename + ".thorn"));

    while (it != lexer::tokenizer_sentinel())
    {
        switch (it->type)
        {
            case lexer::token_type::enum_:
            {
                auto enum_ = parser::parse_enum(filename, it);
                _insert_enum(filename, enum_);
                break;
            }

            case lexer::token_type::struct_:
            {
                auto struct_ = parser::parse_struct(filename, it);
                _insert_struct(filename, struct_);
                break;
            }

            case lexer::token_type::syscall:
            {
                auto syscall = parser::parse_syscall(filename, it);
                _insert_syscall(filename, syscall);
                break;
            }

            case lexer::token_type::include:
            {
                auto include = parser::parse_include(filename, it);
                _insert_include(filename, include);
                break;
            }

            default:
                REPORT_ERROR(
                    filename, it->line, it->column, "expected declaration, got '" << it->contents << "'");
        }
    }
}

void context::output()
{
    std::random_device dev;
    std::mt19937 engine(dev());

    std::shuffle(_syscalls.begin(), _syscalls.end(), engine);

    for (std::size_t i = 0; i < _syscalls.size(); ++i)
    {
        std::get<generator::syscall_description>(_symbols[_syscalls[i]].description).id = i;
    }

    generator::context gen_ctx(
        generator::context_args{ .scope = _args.scope, .arch = _args.arch, .output_dir = _args.output_dir });

    gen_ctx.set_dependencies(_module_deps);
    gen_ctx.set_includes(_module_includes);
    for (auto && name : _symbols_in_order)
    {
        gen_ctx.generate_symbol(name, _symbols[name]);
    }
}

std::optional<std::string> context::_process_qualified_name(
    std::string_view module,
    const parser::qualified_name & ast)
{
    if (ast.in_generated_scope)
    {
        auto it = _symbols.find(ast.name_elements.back());
        if (it == _symbols.end())
        {
            return std::nullopt;
        }

        if (module != it->second.module)
        {
            _module_deps[std::string(module)].insert(it->second.module);
        }
        return std::make_optional(_args.scope + "::" + ast.name_elements.back());
    }

    std::string ret = ast.name_elements.front();
    for (std::size_t i = 1; i < ast.name_elements.size(); ++i)
    {
        ret.append("::");
        ret.append(ast.name_elements[i]);
    }
    return std::make_optional(std::move(ret));
}

void context::_insert_enum(std::string_view module, const parser::parsed_enum & ast)
{
    generator::symbol s;
    s.module = module;

    generator::enum_description desc;
    for (std::uintptr_t i = 0; i < ast.enumerations.size(); ++i)
    {
        desc.enumerations.push_back(std::make_pair(std::move(ast.enumerations[i]), i));
    }

    s.description = std::move(desc);

    _insert_symbol(ast.name, std::move(s));
}

void context::_insert_struct(std::string_view module, const parser::parsed_struct & ast)
{
    generator::symbol s;
    s.module = module;

    generator::struct_description desc;
    for (auto && member : ast.members)
    {
        switch (member.type.index())
        {
            case 0:
            {
                auto qual = std::get<0>(member.type);
                auto processed = _process_qualified_name(module, qual);

                if (!processed)
                {
                    std::cerr << "thorn: semantic error: the definition of member '" << member.name
                              << "' of struct '" << ast.name
                              << "' references '$::" << qual.name_elements.back()
                              << "', which was not defined when its definition was processed" << std::endl;
                    std::exit(1);
                }

                desc.members.push_back(std::make_pair(member.name, std::move(*processed)));

                break;
            }

            case 1:
            {
                auto union_ = std::get<1>(member.type);
                generator::union_description udesc;

                for (auto && umember : union_.members)
                {
                    auto processed = _process_qualified_name(module, umember.type);

                    if (!processed)
                    {
                        std::cerr << "thorn: semantic error: the definition of member '" << member.name
                                  << "' of struct '" << ast.name
                                  << "' references '$::" << umember.type.name_elements.back()
                                  << "', which was not defined when its definition was processed"
                                  << std::endl;
                        std::exit(1);
                    }

                    udesc.members.push_back(std::make_pair(std::move(umember.name), std::move(*processed)));
                }

                desc.members.push_back(std::make_pair(member.name, std::move(udesc)));

                break;
            }

            default:
                std::cerr
                    << "thorn: internal error: invalid index for struct member's type variant (struct: '"
                    << ast.name << "', member: '" << member.name << "'" << std::endl;
                std::exit(1);
        }
    }

    s.description = std::move(desc);

    _insert_symbol(ast.name, std::move(s));
}

void context::_insert_syscall(std::string_view module, const parser::parsed_syscall & ast)
{
    if (ast.parameters.size() > generator::arch_registers_for_args.at(_args.arch).size())
    {
        std::cerr << "thorn: semantic error: syscall '" << ast.name
                  << "' accepts more parameters than supposed on " << _args.arch << std::endl;
        std::exit(1);
    }

    generator::symbol s;
    s.module = module;

    bool has_token_with_permissions = false;

    generator::syscall_description desc;
    desc.implementation_scope = ast.scope.name_elements.front();
    for (std::size_t i = 1; i < ast.scope.name_elements.size(); ++i)
    {
        desc.implementation_scope.append("::");
        desc.implementation_scope.append(ast.scope.name_elements[i]);
    }

    for (auto && param : ast.parameters)
    {
        if (param.token_permissions && param.type_spec.in_generated_scope)
        {
            std::cerr << "thorn: semantic error: the parameter '" << param.name << "' of syscall '"
                      << ast.name
                      << "' defined as a token to a type in the generated scope, which is invalid (it must "
                         "only reference kernel object types)"
                      << std::endl;
            std::exit(1);
        }

        if (param.in || param.out)
        {
            generator::pointer pdesc;

            auto processed = _process_qualified_name(module, param.type_spec);

            if (!processed)
            {
                std::cerr << "thorn: semantic error: the type of parameter '" << param.name
                          << "' of syscall '" << ast.name
                          << "' references '$::" << param.type_spec.name_elements.back()
                          << "', which was not defined when its definition was processed" << std::endl;
                std::exit(1);
            }

            pdesc.in = param.in;
            pdesc.out = param.out;
            pdesc.deref_type = std::move(*processed);

            desc.parameters.push_back(std::make_pair(param.name, std::move(pdesc)));
        }

        else if (param.token_permissions)
        {
            // TODO: semantic validation of permissions before putting them in for the code generator
            generator::handle_token tdesc;

            for (auto && perm : *param.token_permissions)
            {
                tdesc.required_permissions.push_back(perm);
            }

            if (!tdesc.required_permissions.empty())
            {
                has_token_with_permissions = true;
            }

            tdesc.deref_type = param.type_spec.name_elements.front();
            for (std::size_t i = 1; i < param.type_spec.name_elements.size(); ++i)
            {
                tdesc.deref_type.append("::");
                tdesc.deref_type.append(param.type_spec.name_elements[i]);
            }

            desc.parameters.push_back(std::make_pair(param.name, std::move(tdesc)));
        }

        else
        {
            auto processed = _process_qualified_name(module, param.type_spec);

            if (!processed)
            {
                std::cerr << "thorn: semantic error: the type of parameter '" << param.name
                          << "' of syscall '" << ast.name
                          << "' references '$::" << param.type_spec.name_elements.back()
                          << "', which was not defined when its definition was processed" << std::endl;
                std::exit(1);
            }

            desc.parameters.push_back(std::make_pair(param.name, std::move(*processed)));
        }
    }

    if (ast.return_type)
    {
        auto processed = _process_qualified_name(module, *ast.return_type);

        if (!processed)
        {
            std::cerr << "thorn: semantic error: the return type of syscall '" << ast.name
                      << "' references '$::" << ast.return_type->name_elements.back()
                      << "', which was not defined when its definition was processed" << std::endl;
            std::exit(1);
        }

        desc.return_type = std::move(*processed);
    }

    if (has_token_with_permissions && (!desc.return_type || desc.return_type != _args.scope + "::result"))
    {
        std::cerr << "thorn: semantic error: the syscall '" << ast.name
                  << "' accepts a handle token parameter with specified permissions, but does not return the "
                     "generated result type"
                  << std::endl;
        std::exit(1);
    }

    s.description = std::move(desc);

    _insert_symbol(ast.name, std::move(s));
    _syscalls.push_back(ast.name);
}

void context::_insert_include(std::string_view module, const parser::parsed_include & ast)
{
    _module_includes[std::string(module)].insert(ast.header_name);
}

void context::_insert_symbol(const std::string & name, generator::symbol symb)
{
    if (_symbols.contains(name))
    {
        std::cerr << "thorn: error: duplicate symbol '" << name << "'" << std::endl;
        std::exit(1);
    }

    _symbols[name] = std::move(symb);
    _symbols_in_order.push_back(name);
}
}
