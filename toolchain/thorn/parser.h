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

#include "lexer.h"

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace thorn::parser
{
struct qualified_name
{
    bool in_generated_scope = false;
    std::vector<std::string> name_elements;
};

struct parameter
{
    std::string name;
    bool in = false;
    bool out = false;
    std::optional<std::vector<std::string>> token_permissions;
    qualified_name type_spec;
};

struct parsed_syscall
{
    qualified_name scope;
    std::string name;
    std::vector<parameter> parameters;
    std::optional<qualified_name> return_type;
};

struct parsed_enum
{
    std::string name;
    std::vector<std::string> enumerations;
};

struct union_member
{
    std::string name;
    qualified_name type;
};

struct parsed_union
{
    std::vector<union_member> members;
};

struct struct_member
{
    std::string name;
    std::variant<qualified_name, parsed_union> type;
};

struct parsed_struct
{
    std::string name;
    std::vector<struct_member> members;
};

struct parsed_include
{
    std::string header_name;
};

parsed_syscall parse_syscall(std::string_view filename, lexer::tokenizer_iterator & it);
parsed_enum parse_enum(std::string_view filename, lexer::tokenizer_iterator & it);
parsed_struct parse_struct(std::string_view filename, lexer::tokenizer_iterator & it);
parsed_include parse_include(std::string_view filename, lexer::tokenizer_iterator & it);
}
