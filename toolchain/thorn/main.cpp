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

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

int main(int argc, char ** argv)
{
    std::string input_path;
    std::string output_path;
    std::string scope;
    std::string arch;

    for (auto i = 1; i < argc; ++i)
    {
        auto arg = std::string_view(argv[i]);

        if (arg == "-i" || arg == "--input")
        {
            ++i;
            if (i == argc)
            {
                std::cerr << "thorn: no input path provided after " << arg << std::endl;
                return 1;
            }

            input_path = argv[i];
            continue;
        }

        if (arg == "-o" || arg == "--output")
        {
            ++i;
            if (i == argc)
            {
                std::cerr << "thorn: no output path provided after " << arg << std::endl;
                return 1;
            }

            output_path = argv[i];
            continue;
        }

        if (arg == "-s" || arg == "--scope")
        {
            ++i;
            if (i == argc)
            {
                std::cerr << "thorn: no generated scope provided after " << arg << std::endl;
                return 1;
            }

            scope = argv[i];
            continue;
        }

        if (arg == "-a" || arg == "--arch")
        {
            ++i;
            if (i == argc)
            {
                std::cerr << "thorn: no target arch provided after " << arg << std::endl;
                return 1;
            }

            arch = argv[i];
            continue;
        }

        std::cerr << "thorn: unknown command line argument '" << arg << "'" << std::endl;
        return 1;
    }

    if (input_path.empty())
    {
        std::cerr << "thorn: no input path provided" << std::endl;
        return 1;
    }

    if (output_path.empty())
    {
        std::cerr << "thorn: no output path provided" << std::endl;
        return 1;
    }

    if (scope.empty())
    {
        std::cerr << "thorn: no generated scope provided" << std::endl;
        return 1;
    }

    if (arch.empty())
    {
        std::cerr << "thorn: no target arch provided" << std::endl;
        return 1;
    }

    std::error_code ec;
    std::filesystem::create_directories(output_path, ec);
    if (ec)
    {
        std::cerr << "thorn: failed to create the output directory: " << ec.message() << std::endl;
        return 1;
    }

    thorn::context ctx(thorn::context_args{ .scope = std::move(scope),
                                            .arch = std::move(arch),
                                            .input_dir = std::move(input_path),
                                            .output_dir = std::move(output_path) });
    ctx.process("meta");
    ctx.output();
}
