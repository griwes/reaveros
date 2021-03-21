/*
 * Copyright © 2021 Michał 'Griwes' Dominiak
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

#include "config.h"
#include "../cpu/halt.h"
#include "../efi/console.h"

namespace efi_loader
{
// std::string_view::substr throws on out-of-range; we can't do that here
auto substr(std::string_view str, std::size_t pos, std::size_t num = std::string_view::npos)
{
    str.remove_prefix(pos);

    if (num != std::string_view::npos)
    {
        str.remove_suffix(str.size() - num);
    }

    return str;
}

std::string_view config::operator[](std::string_view key) const
{
    std::string_view config = _config_file.buffer.get();

    for (auto pos = config.find('\n'); pos != std::string_view::npos; pos = config.find('\n'))
    {
        auto line = substr(config, 0, pos);
        config.remove_prefix(pos + 1);

        auto colon = line.find(':');
        if (colon == std::string_view::npos)
        {
            console::print(u"[ERR] Malformed config file detected!\n\r");
            halt();
        }

        // ...maybe, just maybe, I'll write a proper parser for this one day
        // but that day isn't today
        if (substr(line, 0, colon) == key)
        {
            return substr(line, line.find_first_not_of(' ', colon + 1));
        }
    }

    console::print(u"[ERR] Failed to find a value for the key `", key, u"` in the config file.\n\r");
    halt();
}
}
