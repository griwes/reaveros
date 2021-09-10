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

#include "../memory/vas.h"
#include "../util/intrusive_ptr.h"

namespace kernel::scheduler
{
class thread;

class process : public util::intrusive_ptrable<process>
{
public:
    process(std::unique_ptr<vm::vas> address_space);

    util::intrusive_ptr<thread> create_thread();

    vm::vas * get_vas()
    {
        return _address_space.get();
    }

private:
    std::unique_ptr<vm::vas> _address_space;
};
}
