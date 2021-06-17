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

#include "idt.h"
#include "irqs.h"

#include <cstdint>
#include <utility>

namespace
{
struct [[gnu::packed]] idt_entry
{
    std::uint16_t offset_low;
    std::uint16_t selector;
    std::uint8_t ist : 3;
    std::uint8_t zero : 5;
    std::uint8_t type : 4;
    std::uint8_t zero1 : 1;
    std::uint8_t dpl : 2;
    std::uint8_t present : 1;
    std::uint16_t offset_middle;
    std::uint32_t offset_high;
    std::uint32_t zero2;
};

struct [[gnu::packed]] idtr_t
{
    std::uint16_t limit;
    idt_entry * address;
};

idt_entry idt[256];
idtr_t idtr;

// no error code
template<std::uint64_t I>
[[gnu::naked]] void isr() requires(I != 8 && (I<10 || I> 14) && I != 17)
{
    asm volatile(
        R"(
            push $0
            push %0
            jmp common_interrupt_stub
        )" ::"i"(I));
}

// error code
template<std::uint64_t I>
[[gnu::naked]] void isr() requires(I == 8 || (I >= 10 && I <= 14) || I == 17)
{
    asm volatile(
        R"(
            push %0
            jmp common_interrupt_stub
        )" ::"i"(I));
}

void setup_idte(
    std::uint8_t id,
    void (*fun)(),
    std::uint16_t selector,
    bool present,
    std::uint8_t dpl,
    std::uint8_t type,
    idt_entry * table,
    std::uint8_t ist = 0)
{
    auto address = reinterpret_cast<std::uint64_t>(fun);

    table[id].zero = 0;
    table[id].zero1 = 0;
    table[id].zero2 = 0;
    table[id].offset_low = address & 0xffff;
    table[id].offset_middle = (address >> 16) & 0xffff;
    table[id].offset_high = (address >> 32) & 0xffffffff;
    table[id].selector = selector;
    table[id].present = present;
    table[id].dpl = dpl;
    table[id].type = type;
    table[id].ist = ist;
}

template<std::size_t I>
void setup_isr()
{
    setup_idte(I, &isr<I>, 0x8, true, 0, 0xe, idt);
}

template<>
void setup_isr<2>()
{
    setup_idte(2, &isr<2>, 0x8, true, 0, 0xe, idt, 1);
}

template<>
void setup_isr<8>()
{
    setup_idte(8, &isr<8>, 0x8, true, 0, 0xe, idt, 2);
}

template<>
void setup_isr<14>()
{
    setup_idte(14, &isr<14>, 0x8, true, 0, 0xe, idt, 3);
}

template<std::size_t... Is>
[[gnu::always_inline]] void setup_isrs(std::index_sequence<Is...>)
{
    (setup_isr<Is>(), ...);
}
}

extern "C" void load_idtr(void *);

namespace kernel::amd64::idt
{
extern "C" void common_interrupt_handler(irq::context ctx)
{
    irq::handle(ctx);
}

void initialize()
{
    setup_isrs(std::make_index_sequence<256>());

    idtr.address = ::idt;
    idtr.limit = 256 * sizeof(idt_entry) - 1;
}

void load()
{
    load_idtr(&idtr);
}
}
