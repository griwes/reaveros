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

#include "../../../time/time.h"
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

    volatile bool interrupt_fired = false;

    using namespace std::literals;
    time::get_high_precision_timer().one_shot(
        50ms, +[](volatile bool * fired) { *fired = true; }, &interrupt_fired);

    asm volatile("sti");

    while (!interrupt_fired)
    {
        asm volatile("hlt");
    }

    std::uint32_t ticks = -1;
    ticks -= lapic::read_timer_counter();

    lapic::write_timer_counter(0);

    log::println(" >> Tick rate estimate: {} per 50ms", ticks);

    auto period = std::chrono::duration_cast<std::chrono::duration<std::uint64_t, std::femto>>(50ms) / ticks;
    log::println(" >> Tick period estimate: {}fs", period.count());
    auto frequency = 1000000000000000ull / period.count();
    log::println(" >> Tick frequency estimate: {}Hz", frequency);
}
}
