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

#pragma once

#include "../util/chained_allocator.h"

#include <memory>

namespace kernel::vm
{
class vas;

std::unique_ptr<vas> create_vas();
std::unique_ptr<vas> adopt_existing_asid(phys_addr_t asid);

class vas : public util::chained_allocatable<vas>
{
public:
    friend std::unique_ptr<vas> create_vas();
    friend std::unique_ptr<vas> adopt_existing_asid(phys_addr_t);

    ~vas();

    phys_addr_t get_asid() const;

private:
    phys_addr_t _asid;
};
}
