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

#include "lapic.h"

#include "../../../util/log.h"
#include "../cpu/lapic.h"

namespace kernel::amd64::lapic_timer
{
void initialize()
{
    log::println(" > Initializing LAPIC timer...");

    log::println(" >> Estimating tick period...");
    lapic::write_timer_divisor(1);
    lapic::write_timer_counter(-1);

    // trigger hpet interrupt

    std::uint32_t ticks = -1;
    ticks -= lapic::read_timer_counter();

    lapic::write_timer_counter(0);
}
}
