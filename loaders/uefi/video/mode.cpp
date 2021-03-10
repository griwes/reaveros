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

#include "mode.h"
#include "../config/config.h"
#include "../efi/console.h"
#include "../efi/efi.h"

#include <cstring>

namespace efi_loader
{
constexpr auto EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID =
    EFI_GUID{ 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } };

struct EFI_PIXEL_BITMASK
{
    std::uint32_t red_mask;
    std::uint32_t green_mask;
    std::uint32_t blue_mask;
    std::uint32_t reserved_mask;
};

enum class EFI_GRAPHICS_PIXEL_FORMAT
{
    pixel_red_green_blue_reserved_8bit_per_color,
    pixel_blue_green_red_reserved_8bit_per_color,
    pixel_bit_mask,
    pixel_blt_only,
    pixel_format_max
};

struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION
{
    std::uint32_t version;
    std::uint32_t horizontal_resolution;
    std::uint32_t vertical_resolution;
    EFI_GRAPHICS_PIXEL_FORMAT pixel_format;
    EFI_PIXEL_BITMASK pixel_information;
    std::uint32_t pixels_per_scan_line;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE
{
    std::uint32_t max_mode;
    std::uint32_t mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION * info;
    std::size_t size_of_info;
    std::uintptr_t frame_buffer_base;
    std::size_t frame_buffer_size;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL;

using EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE = EFIAPI EFI_STATUS (*)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL * self,
    std::uint32_t mode_number,
    std::size_t * size_of_info,
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION ** info);

using EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE =
    EFIAPI EFI_STATUS (*)(EFI_GRAPHICS_OUTPUT_PROTOCOL * self, std::uint32_t mode_number);

using EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT = void (*)();

struct EFI_GRAPHICS_OUTPUT_PROTOCOL
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE query_mode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE set_mode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE * mode;
};

namespace
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL * proto = nullptr;

    auto init_proto()
    {
        if (!proto)
        {
            proto = FIND_PROTOCOL(GRAPHICS_OUTPUT);
            console::print(u" > Found GRAPHICS_OUTPUT protocol.\n\r");
        }

        return proto;
    }
} // namespace

video_mode choose_mode(const config & cfg)
{
    init_proto();
    console::print(u" > ", proto->mode->max_mode, u" video modes available.\n\r");

    auto preference = cfg["resolutions"];
    console::print(u" > Resolution preference: ", preference, u".\n\r");

    std::uint32_t max_horizontal = 0;
    std::uint32_t max_vertical = 0;
    std::uint32_t max_index = 0;

    std::uint32_t preferred_horizontal = 0;
    std::uint32_t preferred_vertical = 0;
    std::uint32_t preferred_priority = -(std::uint32_t)1;
    std::uint64_t preferred_index = -(std::uint64_t)1;

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION * info = nullptr;
    std::size_t info_size = 0;

    for (auto i = 0u; i < proto->mode->max_mode; ++i)
    {
        switch (auto status = proto->query_mode(proto, i, &info_size, &info))
        {
            case EFI_SUCCESS:
                console::print(
                    u" > Video mode ",
                    i,
                    u": ",
                    info->horizontal_resolution,
                    u"x",
                    info->vertical_resolution,
                    u" (pixel format: ",
                    static_cast<int>(info->pixel_format),
                    info->pixel_format != EFI_GRAPHICS_PIXEL_FORMAT::pixel_blt_only ? u", ok" : u", not ok",
                    u").\n\r");

                if (info->pixel_format == EFI_GRAPHICS_PIXEL_FORMAT::pixel_blt_only)
                {
                    // don't want a blt-only format!
                    break;
                }

                if (max_horizontal <= info->horizontal_resolution
                    && max_vertical <= info->vertical_resolution)
                {
                    max_index = i;
                }

                break;

            default:
                console::print(u" > Querying mode ", i, u" failed: ", status, u".\n\r");
        }

        delete info;
    }

    if (preferred_index == -(std::uint64_t)1)
    {
        preferred_index = max_index;
    }

    proto->query_mode(proto, preferred_index, &info_size, &info);

    delete info;

    return {};
}
}
