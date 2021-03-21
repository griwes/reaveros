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

#include "video_mode.h"
#include "../config/config.h"
#include "../cpu/halt.h"
#include "console.h"
#include "efi.h"

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
        }

        return proto;
    }
}

video_mode choose_mode(const config & cfg)
{
    init_proto();

    if (!proto)
    {
        console::print(u" > No graphics protocol found, assuming headless boot.\n\r");
        return {};
    }

    console::print(u" > ", proto->mode->max_mode, u" video modes available.\n\r");

    auto preference = cfg["max-resolution"];
    console::print(u" > Max resolution allowed by config: ", preference, u".\n\r");

    auto x_pos = preference.find('x');
    if (x_pos == std::string_view::npos)
    {
        console::print(u"[ERR] Malformed config file detected, max-resolution does not contain an 'x'!\n\r");
        halt();
    }

    auto horizontal_config = preference.substr(0, x_pos);
    auto vertical_config = preference.substr(x_pos + 1);

    auto parse_int = [](std::string_view input, auto && description)
    {
        bool failed = false;

        std::uint32_t ret = 0;

        for (auto && c : input)
        {
            if (c < '0' || c > '9')
            {
                failed = true;
                break;
            }

            auto new_ret = ret * 10 + (c - '0');
            if (new_ret < ret)
            {
                failed = true;
                break;
            }

            ret = new_ret;
        }

        if (failed)
        {
            console::print(
                u"[ERR] Malformed config file detected, the ",
                description,
                " max-resolution is not a valid unsigned decimal integer that fits in 32 bits!\n\r");
            halt();
        }

        return ret;
    };

    std::uint32_t max_horizontal = parse_int(horizontal_config, u"horizontal");
    std::uint32_t max_vertical = parse_int(vertical_config, u"vertical");

    std::uint32_t preferred_horizontal = 0;
    std::uint32_t preferred_vertical = 0;
    std::uint32_t preferred_index = -1;
    bool found_exact_match = false;

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION * info = nullptr;
    std::size_t info_size = 0;

    for (auto i = 0u; i < proto->mode->max_mode && !found_exact_match; ++i)
    {
        switch (auto status = proto->query_mode(proto, i, &info_size, &info))
        {
            case EFI_SUCCESS:
                if (info->pixel_format == EFI_GRAPHICS_PIXEL_FORMAT::pixel_blt_only)
                {
                    // don't want a blt-only format!
                    break;
                }

                if (max_horizontal == info->horizontal_resolution
                    && max_vertical == info->vertical_resolution)
                {
                    preferred_index = i;
                    found_exact_match = true;
                    break;
                }

                if (max_horizontal < info->horizontal_resolution || max_vertical < info->vertical_resolution)
                {
                    continue;
                }

                if (preferred_horizontal < info->horizontal_resolution
                    || (preferred_horizontal == info->horizontal_resolution
                        && preferred_vertical < info->vertical_resolution))
                {
                    preferred_horizontal = info->horizontal_resolution;
                    preferred_vertical = info->vertical_resolution;
                    preferred_index = i;
                }

                break;

            default:
                console::print(u" > Querying mode ", i, u" failed: ", status, u".\n\r");
        }

        delete info;
    }

    if (preferred_index == -1)
    {
        console::print(u" > No suitable video mode found, proceeding without graphic output.\n\r");
        return {};
    }

    proto->query_mode(proto, preferred_index, &info_size, &info);

    console::print(
        u" > Selected video mode, index ",
        preferred_index,
        u": ",
        info->horizontal_resolution,
        u"x",
        info->vertical_resolution,
        u" (pixel format: ",
        static_cast<int>(info->pixel_format),
        u").\n\r");

    delete info;

    return { true, preferred_index };
}

void set_mode(video_mode & mode)
{
    switch (auto status = proto->set_mode(proto, mode.mode_number))
    {
        case EFI_SUCCESS:
        {
            auto efi_mode = proto->mode;
            auto info = efi_mode->info;

            mode.info.framebuffer_base = efi_mode->frame_buffer_base;
            mode.info.framebuffer_size = efi_mode->frame_buffer_size;

            switch (info->pixel_format)
            {
                case EFI_GRAPHICS_PIXEL_FORMAT::pixel_red_green_blue_reserved_8bit_per_color:
                    mode.info.format = pixel_format::rgb;
                    break;

                case EFI_GRAPHICS_PIXEL_FORMAT::pixel_blue_green_red_reserved_8bit_per_color:
                    mode.info.format = pixel_format::bgr;
                    break;

                case EFI_GRAPHICS_PIXEL_FORMAT::pixel_bit_mask:
                    mode.info.format = pixel_format::mask;
                    mode.info.masks.red = info->pixel_information.red_mask;
                    mode.info.masks.green = info->pixel_information.green_mask;
                    mode.info.masks.blue = info->pixel_information.blue_mask;
                    break;

                default:
                    console::print(u"[ERR] Video mode with invalid pixel format chosen!\n\r");
                    halt();
            }

            mode.info.ppl = info->pixels_per_scan_line;
            mode.info.x = info->horizontal_resolution;
            mode.info.y = info->vertical_resolution;

            console::print(
                u" > Framebuffer base: ", reinterpret_cast<void *>(mode.info.framebuffer_base), u"\n\r");
            console::print(u" > Framebuffer size: ", mode.info.framebuffer_size, u"\n\r");
            console::print(u" > Framebuffer pixels per line: ", mode.info.ppl, u"\n\r");

            return;
        }

        default:
            console::print(u" > Failed to set video mode: ", status, u".\r\n");
            mode.valid = false;
            return;
    }
}
}
