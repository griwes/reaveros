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

#include "vas.h"

namespace kernel::vm
{
std::unique_ptr<vas> create_vas()
{
    auto ret = std::make_unique<vas>();

    ret->_asid = arch::vm::clone_upper_half();

    return ret;
}

std::unique_ptr<vas> adopt_existing_asid(phys_addr_t asid)
{
    auto ret = std::make_unique<vas>();

    ret->_asid = asid;

    return ret;
}

vas::~vas()
{
    PANIC("this requires an amount of work tbh");
}

phys_addr_t vas::get_asid() const
{
    return _asid;
}
}
