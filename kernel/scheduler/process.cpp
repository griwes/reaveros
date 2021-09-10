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

#include "process.h"
#include "thread.h"

namespace kernel::scheduler
{
process::process(std::unique_ptr<vm::vas> address_space) : _address_space(std::move(address_space))
{
}

util::intrusive_ptr<thread> process::create_thread()
{
    auto ret = util::make_intrusive<thread>(util::intrusive_ptr<process>(this));

    ret->timestamp = time::get_high_precision_timer().now();

    return ret;
}
}
