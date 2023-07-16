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

#include <cstring>
#include <memory>

#include "../cpu/halt.h"
#include "console.h"
#include "efi.h"
#include "filesystem.h"
#include "path.h"

namespace efi_loader
{
constexpr auto EFI_LOADED_IMAGE_PROTOCOL_GUID =
    EFI_GUID{ 0x5b1b31a1, 0x9562, 0x11d2, { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

struct EFI_DEVICE_PATH_PROTOCOL;
using EFI_IMAGE_UNLOAD = void (*)();

struct EFI_LOADED_IMAGE_PROTOCOL
{
    std::uint32_t revision;
    EFI_HANDLE parent_handle;
    EFI_SYSTEM_TABLE * system_table;
    EFI_HANDLE device_handle;
    EFI_DEVICE_PATH_PROTOCOL * file_path;
    void * reserved;
    std::uint32_t load_options_size;
    void * load_options;
    void * image_base;
    std::uint64_t image_size;
    EFI_MEMORY_TYPE image_code_type;
    EFI_MEMORY_TYPE image_data_type;
    EFI_IMAGE_UNLOAD unload;
};

constexpr auto EFI_FILE_PROTOCOL_REVISION = 0x00010000;
constexpr auto EFI_FILE_PROTOCOL_REVISION2 = 0x00020000;
constexpr auto EFI_FILE_PROTOCOL_LATEST_REVISION = EFI_FILE_PROTOCOL_REVISION2;

struct EFI_FILE_PROTOCOL;

using EFI_FILE_OPEN = EFIAPI EFI_STATUS (*)(
    EFI_FILE_PROTOCOL * self,
    EFI_FILE_PROTOCOL ** new_handle,
    const char16_t * filename,
    std::uint64_t open_mode,
    std::uint64_t attributes);

constexpr auto EFI_FILE_MODE_READ = 0x0000000000000001;
constexpr auto EFI_FILE_MODE_WRITE = 0x0000000000000002;
constexpr auto EFI_FILE_MODE_CREATE = 0x8000000000000000;

constexpr auto EFI_FILE_READ_ONLY = 0x0000000000000001;
constexpr auto EFI_FILE_HIDDEN = 0x0000000000000002;
constexpr auto EFI_FILE_SYSTEM = 0x0000000000000004;
constexpr auto EFI_FILE_RESERVED = 0x0000000000000008;
constexpr auto EFI_FILE_DIRECTORY = 0x0000000000000010;
constexpr auto EFI_FILE_ARCHIVE = 0x0000000000000020;
constexpr auto EFI_FILE_VALID_ATTR = 0x0000000000000037;

using EFI_FILE_CLOSE = EFIAPI EFI_STATUS (*)(EFI_FILE_PROTOCOL * self);

using EFI_FILE_DELETE = void (*)();

using EFI_FILE_READ =
    EFIAPI EFI_STATUS (*)(EFI_FILE_PROTOCOL * self, std::size_t * buffer_size, void * buffer);

using EFI_FILE_WRITE = void (*)();
using EFI_FILE_GET_POSITION = void (*)();
using EFI_FILE_SET_POSITION = void (*)();

auto EFI_FILE_INFO_ID =
    EFI_GUID{ 0x09576e92, 0x6d3f, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

struct EFI_FILE_INFO
{
    std::uint64_t size;
    std::uint64_t file_size;
    std::uint64_t physical_size;
    EFI_TIME create_time;
    EFI_TIME last_access_time;
    EFI_TIME modification_time;
    std::uint64_t attribute;
    char16_t file_name[8 * 1024];
};

using EFI_FILE_GET_INFO = EFIAPI EFI_STATUS (*)(
    EFI_FILE_PROTOCOL * self,
    EFI_GUID * info_type,
    std::size_t * buffer_size,
    void * buffer);

using EFI_FILE_SET_INFO = void (*)();
using EFI_FILE_FLUSH = void (*)();
using EFI_FILE_OPEN_EX = void (*)();
using EFI_FILE_READ_EX = void (*)();
using EFI_FILE_WRITE_EX = void (*)();
using EFI_FILE_FLUSH_EX = void (*)();

struct EFI_FILE_PROTOCOL
{
    std::uint64_t revision;
    EFI_FILE_OPEN open;
    EFI_FILE_CLOSE close;
    EFI_FILE_DELETE delete_;
    EFI_FILE_READ read;
    EFI_FILE_WRITE write;
    EFI_FILE_GET_POSITION get_position;
    EFI_FILE_SET_POSITION set_position;
    EFI_FILE_GET_INFO get_info;
    EFI_FILE_SET_INFO set_info;
    EFI_FILE_FLUSH flush;
    EFI_FILE_OPEN_EX open_ex;
    EFI_FILE_READ_EX read_ex;
    EFI_FILE_WRITE_EX write_ex;
    EFI_FILE_FLUSH_EX flush_ex;
};

constexpr auto EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID =
    EFI_GUID{ 0x0964e5b22, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

using EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME =
    EFIAPI EFI_STATUS (*)(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL * self, EFI_FILE_PROTOCOL ** root);

constexpr auto EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION = 0x00010000;

struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
{
    std::uint64_t revision;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME open_volume;
};

namespace
{
    EFI_LOADED_IMAGE_PROTOCOL * loaded_image = nullptr;
    EFI_FILE_PROTOCOL * boot_fs_root = nullptr;
}

path locate_source_directory(EFI_HANDLE image_handle)
{
    loaded_image = FIND_PROTOCOL_FROM_HANDLE(image_handle, LOADED_IMAGE);
    console::print(u" > LOADED_IMAGE protocol found.\n\r");

    auto image_path = path{ loaded_image->file_path };
    console::print(u" > Loaded image path: `", image_path, u"`.\n\r");

    auto simple_fs_proto = FIND_PROTOCOL_FROM_HANDLE(loaded_image->device_handle, SIMPLE_FILE_SYSTEM);
    console::print(u" > SIMPLE_FILE_SYSTEM protocol opened.\n\r");

    switch (simple_fs_proto->open_volume(simple_fs_proto, &boot_fs_root))
    {
        case EFI_SUCCESS:
            console::print(u" > Boot filesystem root opened.\n\r");
            break;

        default:
            console::print(u"[ERR] Unknown error happened while opening boot filesystem root.\n\r");
            halt();
    }

    return image_path / u"..";
}

file_buffer load_file(const path & p)
{
    console::print(u" > Trying to open `", p, u"`.\n\r");

    EFI_FILE_PROTOCOL * file = nullptr;
    switch (boot_fs_root->open(boot_fs_root, &file, p.string(), EFI_FILE_MODE_READ, 0))
    {
        case EFI_SUCCESS:
            console::print(u" > Succeeded opening `", p, u"`.\n\r");
            break;

        default:
            console::print(u"[ERR] Failed to open `", p, u"`.\n\r");
            halt();
    }

    EFI_FILE_INFO info;
    std::size_t size = sizeof(info);
    switch (auto status = file->get_info(file, &EFI_FILE_INFO_ID, &size, &info))
    {
        case EFI_SUCCESS:
            console::print(u" > File size of `", p, u"`: ", info.file_size, u".\n\r");
            size = info.file_size;
            break;

        default:
            console::print(u"[ERR] Getting file info for `", p, u"` failed: ", status & ~high_bit, u".\n\r");
            halt();
    }

    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
    std::memset(buffer.get(), '\0', size);

    switch (auto status = file->read(file, &size, buffer.get()))
    {
        case EFI_SUCCESS:
            console::print(u" > File `", p, u"` loaded.\n\r");
            break;

        default:
            console::print(u"[ERR] Loading file `", p, u"` failed: ", status & ~high_bit, u".\n\r");
            halt();
    }

    file->close(file);

    return { size, std::move(buffer) };
}

image_info get_image_info()
{
    image_info ret;

    ret.image_base = loaded_image->image_base;
    ret.image_size = loaded_image->image_size;

    return ret;
}
}
