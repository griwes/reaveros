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

#include <cstddef>
#include <cstdint>

namespace boot_protocol
{
enum class pixel_format
{
    rgb,
    bgr,
    mask
};

struct pixel_format_masks
{
    std::uint32_t red;
    std::uint32_t green;
    std::uint32_t blue;
};

struct video_mode
{
    void * framebuffer_base;
    std::size_t framebuffer_size;
    std::uint32_t x;
    std::uint32_t y;
    std::uint32_t ppl;
    pixel_format format;
    pixel_format_masks masks;
};
}
