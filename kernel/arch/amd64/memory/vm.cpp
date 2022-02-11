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

#include "vm.h"

#include "../../../memory/pmm.h"
#include "../../../memory/vas.h"
#include "../../../util/bit_lock.h"
#include "../../../util/log.h"
#include "../../../util/mp.h"
#include "../../../util/pointer_types.h"
#include "../cpu/cpu.h"

namespace kernel::amd64::vm
{
void set_asid(phys_addr_t asid)
{
    asm volatile("mov %0, %%cr3" ::"r"(asid.value()) : "memory");
}

phys_addr_t get_asid()
{
    std::uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return phys_addr_t(cr3);
}

namespace
{
    template<int I>
    struct pmlt;

    template<int I>
    struct pmle
    {
        pmlt<I - 1> * get() const
        {
            return phys_ptr_t<pmlt<I - 1>>(phys_addr_t(address << 12)).value();
        }

        phys_addr_t get_phys() const
        {
            return phys_addr_t(address << 12);
        }

        void operator=(pmlt<I - 1> * phys)
        {
            present = 1;
            address = phys_ptr_t(phys).representation().value() >> 12;
        }

        void operator=(std::uintptr_t phys)
        {
            present = 1;
            size = 1;
            address = phys >> 12;
        }

        std::uint64_t present : 1 = 0;
        std::uint64_t read_write : 1 = 1;
        std::uint64_t user : 1;
        std::uint64_t write_through : 1;
        std::uint64_t cache_disable : 1;
        std::uint64_t accessed : 1;
        std::uint64_t ignored : 1;
        std::uint64_t size : 1;
        std::uint64_t global : 1 = 1;
        std::uint64_t ignored2 : 3;
        std::uint64_t address : 40;
        std::uint64_t ignored3 : 10;
        std::uint64_t lock_bit : 1;
        std::uint64_t reserved2 : 1;
    };

    template<>
    struct pmle<1>
    {
        phys_addr_t get_phys() const
        {
            return phys_addr_t(address << 12);
        }

        void operator=(std::uintptr_t phys)
        {
            present = 1;
            address = phys >> 12;
        }

        std::uint64_t present : 1 = 0;
        std::uint64_t read_write : 1 = 1;
        std::uint64_t user : 1;
        std::uint64_t write_through : 1;
        std::uint64_t cache_disable : 1;
        std::uint64_t accessed : 1;
        std::uint64_t dirty : 1;
        std::uint64_t pat : 1;
        std::uint64_t global : 1 = 1;
        std::uint64_t ignored : 3;
        std::uint64_t address : 40;
        std::uint64_t ignored2 : 10;
        std::uint64_t lock_bit : 1;
        std::uint64_t reserved : 1;
    };

    template<int I>
    struct pmlt
    {
        pmle<I> entries[512]{};

        void * operator new(std::size_t)
        {
            return phys_ptr_t<void *>(pmm::pop(0)).value();
        }

        void operator delete(void * ptr, std::size_t)
        {
            pmm::push(0, phys_ptr_t(ptr).representation());
        }
    };

    using pml4_t = pmlt<4>;

    template<int I, int Lowest>
    [[gnu::always_inline]] void vm_map(
        pmlt<I> * table,
        std::uintptr_t virt_start,
        std::uintptr_t virt_end,
        std::uintptr_t phys,
        kernel::vm::flags flags)
    {
        auto start_table_index = (virt_start >> (I * 9 + 3)) & 511;

        constexpr auto entry_size = 1ull << (I * 9 + 3);

        while (virt_start < virt_end)
        {
            util::bit_lock<62> _(&table->entries[start_table_index]);

            auto entry_virt_end = (virt_start + entry_size) & ~(entry_size - 1);
            entry_virt_end = (entry_virt_end - 1) < virt_end && entry_virt_end ? entry_virt_end : virt_end;
            //                                 ^ this is a protection against overflow on highest addresses

            if constexpr (I == Lowest)
            {
                if (table->entries[start_table_index].present)
                {
                    PANIC(
                        "Tried to re-map page {:#018x} at level {}, existing mapping: {:#018x}!",
                        virt_start,
                        I,
                        table->entries[start_table_index].get_phys().value());
                }

                table->entries[start_table_index] = phys;
                table->entries[start_table_index].user = flags & kernel::vm::flags::user;
            }

            else
            {
                if (table->entries[start_table_index].present == 0)
                {
                    table->entries[start_table_index] = new pmlt<I - 1>{};
                }

                table->entries[start_table_index].user |= flags & kernel::vm::flags::user;

                vm_map<I - 1, Lowest>(
                    table->entries[start_table_index].get(), virt_start, entry_virt_end, phys, flags);
            }

            ++start_table_index;
            virt_start = entry_virt_end;
            phys = (phys + entry_size) & ~(entry_size - 1);
        }
    }

    template<int I>
    [[gnu::always_inline]] void vm_unmap(
        pmlt<I> * table,
        std::uintptr_t virt_start,
        std::uintptr_t virt_end,
        bool free_physical)
    {
        auto start_table_index = (virt_start >> (I * 9 + 3)) & 511;

        constexpr auto entry_size = 1ull << (I * 9 + 3);

        while (virt_start < virt_end)
        {
            util::bit_lock<62> _(&table->entries[start_table_index]);

            auto entry_virt_end = (virt_start + entry_size) & ~(entry_size - 1);
            entry_virt_end = (entry_virt_end - 1) < virt_end && entry_virt_end ? entry_virt_end : virt_end;
            //                                 ^ this is a protection against overflow on highest addresses

            if constexpr (I == 1)
            {
                if (table->entries[start_table_index].present == 0)
                {
                    PANIC("Tried to unmap an unmapped address {:#018x}!", virt_start);
                }

                if (free_physical)
                {
                    auto frame = table->entries[start_table_index].get_phys();
                    pmm::push(0, frame);
                }

                table->entries[start_table_index].present = false;
            }

            else
            {
                if (table->entries[start_table_index].present == 0)
                {
                    PANIC("Tried to unmap an unmapped address {:#018x}!", virt_start);
                }

                if (table->entries[start_table_index].size == 1)
                {
                    PANIC("tried to unmap large page, this is not implemented yet ({:#018x})", virt_start);
                }

                vm_unmap<I - 1>(
                    table->entries[start_table_index].get(), virt_start, entry_virt_end, free_physical);
            }

            ++start_table_index;
            virt_start = entry_virt_end;
        }
    }

    template<int I>
    [[gnu::always_inline]] phys_addr_t vm_virt_to_phys(pmlt<I> * table, std::uintptr_t virt)
    {
        auto table_index = (virt >> (I * 9 + 3)) & 511;

        if (table->entries[table_index].present == 0)
        {
            PANIC("Tried to probe an unmapped address ({:#018x})!", virt);
        }

        if constexpr (I == 1)
        {
            return table->entries[table_index].get_phys();
        }

        else
        {
            if (table->entries[table_index].size == 1)
            {
                return table->entries[table_index].get_phys();
            }

            return vm_virt_to_phys<I - 1>(table->entries[table_index].get(), virt);
        }
    }

    constexpr auto page_size = 4ull * 1024;
    constexpr auto large_page_size = 512 * page_size;
    constexpr auto huge_page_size = 512 * large_page_size;

    constexpr auto page_mask = ~(page_size - 1);
    constexpr auto large_page_mask = ~(large_page_size - 1);
    constexpr auto huge_page_mask = ~(huge_page_size - 1);
}

void map_physical(virt_addr_t start, virt_addr_t end, phys_addr_t physical, kernel::vm::flags flags)
{
    map_physical(nullptr, start, end, physical, flags);
}

void map_physical(
    kernel::vm::vas * address_space,
    virt_addr_t start,
    virt_addr_t end,
    phys_addr_t physical,
    kernel::vm::flags flags)
{
    auto size = end.value() - start.value();

    if (size == 0)
    {
        return;
    }

    auto virt_start = start.value() & page_mask;
    auto virt_end = (virt_start + size + page_size - 1) & page_mask;

    auto phys_int = physical.value() & page_mask;

    auto cr3 = phys_ptr_t<pml4_t>(address_space ? address_space->get_asid() : get_asid()).value();

    vm_map<4, 1>(cr3, virt_start, virt_end, phys_int, flags);
}

void unmap(virt_addr_t start, virt_addr_t end, bool free_physical)
{
    unmap(nullptr, start, end, free_physical);
}

void unmap(kernel::vm::vas * address_space, virt_addr_t start, virt_addr_t end, bool free_physical)
{
    auto size = end.value() - start.value();

    if (size == 0)
    {
        return;
    }

    auto virt_start = start.value() & page_mask;
    auto virt_end = (virt_start + size + page_size - 1) & page_mask;

    auto cr3 = phys_ptr_t<pml4_t>(address_space ? address_space->get_asid() : get_asid()).value();

    vm_unmap<4>(cr3, virt_start, virt_end, free_physical);
}

phys_addr_t virt_to_phys(virt_addr_t address)
{
    return virt_to_phys(nullptr, address);
}

phys_addr_t virt_to_phys(kernel::vm::vas * address_space, virt_addr_t address)
{
    auto cr3 = phys_ptr_t<pml4_t>(address_space ? address_space->get_asid() : get_asid()).value();

    return vm_virt_to_phys<4>(cr3, address.value());
}

namespace
{
    template<int I>
    [[gnu::always_inline]] void unmap_all(pmlt<I> * table, std::size_t first, std::size_t last)
    {
        if constexpr (I != 1)
        {
            while (first <= last)
            {
                if (table->entries[first].lock_bit)
                {
                    PANIC("Page table lock set while unmapping lower half!");
                }

                if (table->entries[first].present == 1)
                {
                    if (!table->entries[first].size)
                    {
                        unmap_all(table->entries[first].get(), 0, 511);
                    }
                    table->entries[first].present = 0;
                    pmm::push(0, table->entries[first].get_phys());
                }

                ++first;
            }
        }
    }

    std::atomic<bool> unmap_lower_half_called = false;
}

phys_addr_t clone_upper_half()
{
    auto ret = new pml4_t;
    auto cr3 = phys_ptr_t<pml4_t>(get_asid());

    for (int i = 256; i < 512; ++i)
    {
        util::bit_lock<62> _(&cr3->entries[i]);

        if (!cr3->entries[i].present)
        {
            cr3->entries[i] = new pmlt<3>;
        }

        ret->entries[i] = cr3->entries[i];
    }

    return phys_ptr_t<pml4_t>(ret).representation();
}

void unmap_lower_half()
{
    if (unmap_lower_half_called.exchange(true))
    {
        PANIC("arch::vm::unmap_lower_half called more than once!");
    }

    auto cr3 = phys_ptr_t<pml4_t>(get_asid()).value();

    unmap_all(cr3, 0, 255);

    kernel::mp::parallel_execute([] { asm volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "memory"); });
}
}
