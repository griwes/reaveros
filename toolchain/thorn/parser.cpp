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

#include "parser.h"

namespace thorn::parser
{
namespace
{
    std::string expect(std::string_view filename, lexer::tokenizer_iterator & it, lexer::token_type type)
    {
        if (it->type != type)
        {
            REPORT_ERROR(
                filename,
                it->line,
                it->column,
                "expected " << lexer::descriptions.at(type) << ", got '" << it->contents << "'");
        }

        auto ret = std::move(it->contents);
        ++it;
        return ret;
    }
}

qualified_name parse_qualified_name(std::string_view filename, lexer::tokenizer_iterator & it)
{
    qualified_name ret;

    if (it->type == lexer::token_type::dollar)
    {
        ret.in_generated_scope = true;
        expect(filename, it, lexer::token_type::dollar);
        expect(filename, it, lexer::token_type::scope);
    }
    ret.name_elements.push_back(expect(filename, it, lexer::token_type::identifier));
    while (it->type == lexer::token_type::scope)
    {
        expect(filename, it, lexer::token_type::scope);
        ret.name_elements.push_back(expect(filename, it, lexer::token_type::identifier));
    }

    if (ret.in_generated_scope && ret.name_elements.size() != 1)
    {
        REPORT_ERROR(
            filename,
            it->line,
            it->column,
            "a qualified name referencing generated scope contains more than one nested name segment");
    }

    return ret;
}

permissions_definition parse_permissions(std::string_view filename, lexer::tokenizer_iterator & it)
{
    permissions_definition ret;

    expect(filename, it, lexer::token_type::permissions);
    if (it->type != lexer::token_type::open_paren)
    {
        expect(filename, it, lexer::token_type::for_);
        ret.object_type = parse_qualified_name(filename, it);
    }

    expect(filename, it, lexer::token_type::open_paren);
    ret.permissions.push_back(expect(filename, it, lexer::token_type::identifier));
    while (it->type != lexer::token_type::close_paren)
    {
        expect(filename, it, lexer::token_type::comma);
        ret.permissions.push_back(expect(filename, it, lexer::token_type::identifier));
    }
    expect(filename, it, lexer::token_type::close_paren);

    expect(filename, it, lexer::token_type::semicolon);

    return ret;
}

parameter parse_parameter(std::string_view filename, lexer::tokenizer_iterator & it)
{
    parameter ret;

    ret.name = expect(filename, it, lexer::token_type::identifier);
    expect(filename, it, lexer::token_type::colon);

    while (it->type == lexer::token_type::direction)
    {
        if (it->contents == "in")
        {
            ret.in = true;
        }
        if (it->contents == "out")
        {
            ret.out = true;
        }
        expect(filename, it, lexer::token_type::direction);
    }

    if (ret.in || ret.out)
    {
        expect(filename, it, lexer::token_type::ptr);
    }

    if (it->type == lexer::token_type::token)
    {
        ret.token_permissions.emplace();

        expect(filename, it, lexer::token_type::token);
        expect(filename, it, lexer::token_type::open_paren);
        ret.token_permissions->push_back(expect(filename, it, lexer::token_type::identifier));
        while (it->type != lexer::token_type::close_paren)
        {
            expect(filename, it, lexer::token_type::comma);
            ret.token_permissions->push_back(expect(filename, it, lexer::token_type::identifier));
        }
        expect(filename, it, lexer::token_type::close_paren);
    }

    ret.type_spec = parse_qualified_name(filename, it);

    return ret;
}

parsed_syscall parse_syscall(std::string_view filename, lexer::tokenizer_iterator & it)
{
    parsed_syscall ret;

    expect(filename, it, lexer::token_type::syscall);
    expect(filename, it, lexer::token_type::open_paren);
    ret.scope = parse_qualified_name(filename, it);

    if (it->type == lexer::token_type::comma)
    {
        expect(filename, it, lexer::token_type::comma);
        expect(filename, it, lexer::token_type::blocking);
        ret.blocking = true;
    }

    expect(filename, it, lexer::token_type::close_paren);
    ret.name = expect(filename, it, lexer::token_type::identifier);

    expect(filename, it, lexer::token_type::open_paren);
    ret.parameters.push_back(parse_parameter(filename, it));
    while (it->type != lexer::token_type::close_paren)
    {
        expect(filename, it, lexer::token_type::comma);
        ret.parameters.push_back(parse_parameter(filename, it));
    }

    expect(filename, it, lexer::token_type::close_paren);

    if (it->type == lexer::token_type::arrow)
    {
        expect(filename, it, lexer::token_type::arrow);
        ret.return_type = parse_qualified_name(filename, it);
    }

    expect(filename, it, lexer::token_type::semicolon);

    return ret;
}

parsed_enum parse_enum(std::string_view filename, lexer::tokenizer_iterator & it)
{
    parsed_enum ret;

    expect(filename, it, lexer::token_type::enum_);
    ret.name = expect(filename, it, lexer::token_type::identifier);

    expect(filename, it, lexer::token_type::open_paren);
    ret.enumerations.push_back(expect(filename, it, lexer::token_type::identifier));
    while (it->type != lexer::token_type::close_paren)
    {
        expect(filename, it, lexer::token_type::comma);
        ret.enumerations.push_back(expect(filename, it, lexer::token_type::identifier));
    }
    expect(filename, it, lexer::token_type::close_paren);

    expect(filename, it, lexer::token_type::semicolon);

    return ret;
}

union_member parse_union_member(std::string_view filename, lexer::tokenizer_iterator & it)
{
    union_member ret;

    ret.name = expect(filename, it, lexer::token_type::identifier);
    expect(filename, it, lexer::token_type::colon);
    ret.type = parse_qualified_name(filename, it);
    return ret;
}

parsed_union parse_union(std::string_view filename, lexer::tokenizer_iterator & it)
{
    parsed_union ret;

    expect(filename, it, lexer::token_type::union_);

    expect(filename, it, lexer::token_type::open_paren);
    ret.members.push_back(parse_union_member(filename, it));
    while (it->type != lexer::token_type::close_paren)
    {
        expect(filename, it, lexer::token_type::comma);
        ret.members.push_back(parse_union_member(filename, it));
    }
    expect(filename, it, lexer::token_type::close_paren);

    return ret;
}

struct_member parse_struct_member(std::string_view filename, lexer::tokenizer_iterator & it)
{
    struct_member ret;

    ret.name = expect(filename, it, lexer::token_type::identifier);
    expect(filename, it, lexer::token_type::colon);
    if (it->type == lexer::token_type::union_)
    {
        ret.type = parse_union(filename, it);
    }
    else
    {
        ret.type = parse_qualified_name(filename, it);
    }

    return ret;
}

parsed_struct parse_struct(std::string_view filename, lexer::tokenizer_iterator & it)
{
    parsed_struct ret;

    expect(filename, it, lexer::token_type::struct_);
    ret.name = expect(filename, it, lexer::token_type::identifier);

    expect(filename, it, lexer::token_type::open_paren);
    while (it->type != lexer::token_type::close_paren)
    {
        ret.members.push_back(parse_struct_member(filename, it));
        if (it->type != lexer::token_type::close_paren)
        {
            expect(filename, it, lexer::token_type::comma);
        }
    }
    expect(filename, it, lexer::token_type::close_paren);

    expect(filename, it, lexer::token_type::semicolon);

    return ret;
}

parsed_include parse_include(std::string_view filename, lexer::tokenizer_iterator & it)
{
    parsed_include ret;

    expect(filename, it, lexer::token_type::include);
    ret.header_name = expect(filename, it, lexer::token_type::header_name);
    expect(filename, it, lexer::token_type::semicolon);

    return ret;
}
}
