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

#include "scheduler.h"

#include "../arch/cpu.h"
#include "../util/log.h"
#include "../util/mp.h"

namespace kernel::scheduler
{
void initialize()
{
    log::println("[SCHED] Initializing scheduler...");
    mp::parallel_execute(
        mp::policy::all,
        +[] { log::println(" > Hello from CPU core #{}!", arch::cpu::get_current_core().id()); });
}
}
