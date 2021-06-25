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

#include "../../../time/time.h"

namespace kernel::amd64::lapic_timer
{
void initialize();

class timer : public time::timer
{
public:
    void bsp_initialize();
    void initialize(timer * bsp);

protected:
    virtual void _update_now() override final;
    virtual void _one_shot_after(std::chrono::nanoseconds) override final;

private:
    std::chrono::time_point<time::timer, std::chrono::duration<__int128, std::femto>> _fine_now;
    std::chrono::duration<std::int64_t, std::femto> _period;
    std::uint32_t _last_written = 0;
    std::uint8_t _last_divisor = 0;
};
}
