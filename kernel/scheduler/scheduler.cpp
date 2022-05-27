/*
 * Copyright © 2021-2022 Michał 'Griwes' Dominiak
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
#include "../arch/irqs.h"
#include "../util/log.h"
#include "../util/mp.h"
#include "thread.h"

namespace kernel::scheduler
{
namespace
{
    aggregate global_scheduler;
    util::intrusive_ptr<process> kernel_process;
    std::atomic<bool> initialized = false;
}

void initialize()
{
    log::println("[SCHED] Initializing scheduler...");

    auto kernel_vas = vm::adopt_existing_asid(arch::vm::get_asid());
    kernel_process = util::make_intrusive<process>(std::move(kernel_vas));

    for (std::size_t i = 0; i < arch::cpu::get_core_count(); ++i)
    {
        auto core = arch::cpu::get_core_by_id(i);
        core->get_scheduler()->initialize(&global_scheduler, core);
        core->get_core_local_storage()->current_thread = core->get_scheduler()->get_idle_thread();
    }

    arch::irq::register_handler(
        arch::irq::scheduling_trigger,
        +[](arch::irq::context &) { arch::cpu::get_current_core()->get_scheduler()->scheduling_trigger(); });

    initialized.store(true, std::memory_order_relaxed);
}

bool is_initialized()
{
    return initialized.load(std::memory_order_relaxed);
}

void schedule(util::intrusive_ptr<thread> thread)
{
    global_scheduler.schedule(std::move(thread));
}

void post_schedule(util::intrusive_ptr<thread> thread)
{
    if (arch::cpu::interrupts_disabled())
    {
        PANIC("scheduler::post_schedule called with interrupts disabled (it's meant to only be called "
              "outside of interrupt handlers)!");
    }

    kernel::mp::parallel_execute(
        kernel::mp::policy::specific,
        +[](kernel::util::intrusive_ptr<kernel::scheduler::thread> * thread)
        { kernel::scheduler::schedule(std::move(*thread)); },
        &thread,
        kernel::arch::cpu::get_current_core()->id());
}

util::intrusive_ptr<process> get_kernel_process()
{
    return kernel_process;
}

util::intrusive_ptr<process> create_process(util::intrusive_ptr<vm::vas> address_space)
{
    if (!address_space->claim_for_process())
    {
        return {};
    }

    auto ret = util::make_intrusive<process>(std::move(address_space));

    return ret;
}
}
