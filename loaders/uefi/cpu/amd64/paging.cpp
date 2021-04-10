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

#include "paging.h"
#include "../../efi/console.h"
#include "../../efi/efi.h"
#include "halt.h"

#include <cstddef>
#include <cstdint>

namespace efi_loader::inline amd64
{
template<int I>
struct pmlt;

template<int I>
struct pmle
{
    pmlt<I - 1> * get() const
    {
        return reinterpret_cast<pmlt<I - 1> *>(address << 12);
    }

    void operator=(pmlt<I - 1> * phys)
    {
        present = 1;
        address = reinterpret_cast<std::uintptr_t>(phys) >> 12;
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

    void * operator new(std::size_t size)
    {
        return allocate_pages(size, EFI_MEMORY_TYPE::reaveros_paging);
    }

    void operator delete(void * ptr, std::size_t size)
    {
        deallocate_pages(ptr, size);
    }
};

using pml4_t = pmlt<4>;

pml4_t * pml4;

void prepare_paging()
{
    pml4 = new pml4_t{};
}

template<int I, int Lowest>
[[gnu::always_inline]] void _vm_map(
    pmlt<I> * table,
    std::uintptr_t virt_start,
    std::uintptr_t virt_end,
    std::uintptr_t phys)
{
    auto start_table_index = (virt_start >> (I * 9 + 3)) & 511;

    constexpr auto entry_size = 1ull << (I * 9 + 3);

    /*
    console::print(
        reinterpret_cast<void *>(virt_start),
        u"-",
        reinterpret_cast<void *>(virt_end),
        u"=",
        reinterpret_cast<void *>(phys),
        u" ",
        start_table_index,
        u"\n\r");
    */

    while (virt_start < virt_end)
    {
        auto entry_virt_end = (virt_start + entry_size) & ~(entry_size - 1);
        entry_virt_end = entry_virt_end < virt_end ? entry_virt_end : virt_end;

        if constexpr (I == Lowest)
        {
            if (table->entries[start_table_index].present)
            {
                console::print(
                    u"[ERR] Tried to re-map page ",
                    reinterpret_cast<void *>(virt_start),
                    u" at level ",
                    I,
                    u", existing mapping: ",
                    reinterpret_cast<void *>(table->entries[start_table_index].address << 12),
                    u"\n\r");
                halt();
            }

            table->entries[start_table_index] = phys;
        }

        else
        {
            if (table->entries[start_table_index].present == 0)
            {
                table->entries[start_table_index] = new pmlt<I - 1>{};
            }

            _vm_map<I - 1, Lowest>(table->entries[start_table_index].get(), virt_start, entry_virt_end, phys);
        }

        ++start_table_index;
        virt_start = entry_virt_end;
        phys = (phys + entry_size) & ~(entry_size - 1);
    }
}

constexpr auto page_size = 4ull * 1024;
constexpr auto large_page_size = 512 * page_size;
constexpr auto huge_page_size = 512 * large_page_size;

constexpr auto page_mask = ~(page_size - 1);
constexpr auto large_page_mask = ~(large_page_size - 1);
constexpr auto huge_page_mask = ~(huge_page_size - 1);

void vm_map(void * phys, std::size_t size, std::uintptr_t virt)
{
    if (size == 0)
    {
        return;
    }

    auto virt_start = virt & page_mask;
    auto virt_end = (virt_start + size + page_size - 1) & page_mask;

    auto phys_int = reinterpret_cast<std::uintptr_t>(phys) & page_mask;

    _vm_map<4, 1>(pml4, virt_start, virt_end, phys_int);
}

void vm_map_large(void * phys, std::size_t size, std::uintptr_t virt)
{
    if (size == 0)
    {
        return;
    }

    auto virt_start = virt & large_page_mask;
    auto virt_end = (virt_start + size + large_page_size - 1) & large_page_mask;

    auto phys_int = reinterpret_cast<std::uintptr_t>(phys) & large_page_mask;

    _vm_map<4, 2>(pml4, virt_start, virt_end, phys_int);
}

void vm_map_huge(void * phys, std::size_t size, std::uintptr_t virt)
{
    if (size == 0)
    {
        return;
    }

    auto virt_start = virt & huge_page_mask;
    auto virt_end = (virt_start + size + huge_page_size - 1) & huge_page_mask;

    auto phys_int = reinterpret_cast<std::uintptr_t>(phys) & huge_page_mask;

    _vm_map<4, 3>(pml4, virt_start, virt_end, phys_int);
}
}
