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

#include "log.h"
#include "boot-constants.h"
#include "boot-memmap.h"

#ifdef __amd64__
#include "../arch/amd64/boot/serial.h"
#else
#error unknown architecture
#endif

#include "../boot/screen.h"

namespace kernel::boot_log
{
struct buffer_chunk
{
    buffer_chunk * previous = nullptr;
    char buffer[2 * 1024 * 1024 - sizeof(previous)]{}; // NOLINT(bugprone-sizeof-expression)
};

namespace
{
    buffer_chunk * latest_chunk = nullptr;
    char * log_cursor = nullptr;
}

void initialize(std::size_t memmap_size, boot_protocol::memory_map_entry * memmap)
{
    auto log_buffer_entry =
        boot_protocol::find_entry(memmap_size, memmap, boot_protocol::memory_type::log_buffer);
    latest_chunk = reinterpret_cast<buffer_chunk *>(
        reinterpret_cast<std::uint8_t *>(log_buffer_entry->physical_start) + boot_protocol::physmem_base);

    log_cursor = latest_chunk->buffer;
}

iterator::iterator()
{
}

iterator::~iterator()
{
}

const iterator::proxy & iterator::proxy::operator=(char c) const
{
    *log_cursor++ = c;
    if (log_cursor == std::end(latest_chunk->buffer))
    {
        // allocate more, but need PMM for that
        asm volatile("ud2");
    }

    boot_screen::put_char(c);
    boot_serial::put_char(c);

    return *this;
}
}

namespace kernel::log
{
std::mutex log_lock;

void * get_syslog_mailbox()
{
    return nullptr;
}
}
