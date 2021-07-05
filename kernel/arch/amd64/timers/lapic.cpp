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
#include "../cpu/cpu.h"
#include "../cpu/irqs.h"
#include "../cpu/lapic.h"

namespace kernel::amd64::lapic_timer
{
namespace
{
    lapic_timer::timer * bsp_timer;
}

void initialize()
{
    bsp_timer = cpu::get_current_core()->timer();
    bsp_timer->bsp_initialize();

    irq::register_handler(
        irq::lapic_timer, +[](irq::context &) { time::timer::handle(cpu::get_current_core()->timer()); });
}

void timer::bsp_initialize()
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

    asm volatile("cli");

    std::uint32_t ticks = -1;
    ticks -= lapic::read_timer_counter();

    lapic::write_timer_counter(0);

    log::println(" >> Tick rate estimate: {} per 50ms.", ticks);

    _period = std::chrono::duration_cast<std::chrono::duration<std::int64_t, std::femto>>(50ms) / ticks;
    log::println(" >> Tick period estimate: {}fs.", _period.count());
    auto frequency = static_cast<std::uint64_t>(std::femto::den) / _period.count();
    log::println(" >> Tick frequency estimate: {}Hz.", frequency);
}

void timer::_update_now()
{
    auto current = lapic::read_timer_counter();
    auto last = std::exchange(_last_written, current);

    _fine_now += static_cast<std::int64_t>(last - current) * _last_divisor * _period;
    _now = std::chrono::time_point<time::timer>{ std::chrono::duration_cast<std::chrono::nanoseconds>(
        _fine_now.time_since_epoch()) };
}

void timer::_one_shot_after(std::chrono::nanoseconds duration_ns)
{
    std::uint8_t divisor = 1;
    while ((duration_ns / (divisor * _period)) > ~static_cast<std::uint32_t>(0) && divisor != 128)
    {
        divisor *= 2;
    }

    auto count = duration_ns / (divisor * _period);
    if (count > ~0u)
    {
        count = ~0u;
    }

    lapic::write_timer_divisor(divisor);
    lapic::write_timer_counter(count);

    _last_written = count;
    _last_divisor = divisor;
}
}
