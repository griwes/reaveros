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

#include "vm.h"

#include "../../../memory/pmm.h"
#include "../../../util/log.h"
#include "../../../util/pointer_types.h"
#include "../cpu/cpu.h"

namespace kernel::amd64::vm
{
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
        std::uint64_t ignored3 : 11;
        std::uint64_t reserved2 : 1;
    };

    template<>
    struct pmle<1>
    {
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
        std::uint64_t ignored2 : 11;
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
        std::uintptr_t phys)
    {
        auto start_table_index = (virt_start >> (I * 9 + 3)) & 511;

        constexpr auto entry_size = 1ull << (I * 9 + 3);

        while (virt_start < virt_end)
        {
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
                        table->entries[start_table_index].address << 12);
                }

                table->entries[start_table_index] = phys;
            }

            else
            {
                if (table->entries[start_table_index].present == 0)
                {
                    table->entries[start_table_index] = new pmlt<I - 1>{};
                }

                vm_map<I - 1, Lowest>(
                    table->entries[start_table_index].get(), virt_start, entry_virt_end, phys);
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
            auto entry_virt_end = (virt_start + entry_size) & ~(entry_size - 1);
            entry_virt_end = (entry_virt_end - 1) < virt_end && entry_virt_end ? entry_virt_end : virt_end;
            //                                 ^ this is a protection against overflow on highest addresses

            if constexpr (I == 2)
            {
                if (table->entries[start_table_index].present == 0)
                {
                    PANIC("Tried to unmap an unmapped address {:#018x}!", virt_start);
                }

                if (free_physical)
                {
                    auto frame = table->entries[start_table_index].get();
                    pmm::push(0, phys_addr_t(reinterpret_cast<std::uintptr_t>(frame)));
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

    constexpr auto page_size = 4ull * 1024;
    constexpr auto large_page_size = 512 * page_size;
    constexpr auto huge_page_size = 512 * large_page_size;

    constexpr auto page_mask = ~(page_size - 1);
    constexpr auto large_page_mask = ~(large_page_size - 1);
    constexpr auto huge_page_mask = ~(huge_page_size - 1);
}

void map_physical(virt_addr_t start, virt_addr_t end, phys_addr_t physical)
{
    auto size = end.value() - start.value();

    if (size == 0)
    {
        return;
    }

    auto virt_start = start.value() & page_mask;
    auto virt_end = (virt_start + size + page_size - 1) & page_mask;

    auto phys_int = physical.value() & page_mask;

    auto cr3 = phys_ptr_t<pml4_t>(cpu::get_asid()).value();

    vm_map<4, 1>(cr3, virt_start, virt_end, phys_int);
}

void unmap(virt_addr_t start, virt_addr_t end, bool free_physical)
{
    auto size = end.value() - start.value();

    if (size == 0)
    {
        return;
    }

    auto virt_start = start.value() & page_mask;
    auto virt_end = (virt_start + size + page_size - 1) & page_mask;

    auto cr3 = phys_ptr_t<pml4_t>(cpu::get_asid()).value();

    vm_unmap<4>(cr3, virt_start, virt_end, free_physical);
}
}
