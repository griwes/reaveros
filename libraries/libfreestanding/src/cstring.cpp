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

#include "../include/cstring"

namespace std
{
extern "C"
{
    void * memcpy(void * s1, const void * s2, size_t n)
    {
        auto s1_u8 = reinterpret_cast<uint8_t *>(s1);
        auto s2_u8 = reinterpret_cast<const uint8_t *>(s2);

        while (n--)
        {
            *s1_u8++ = *s2_u8++;
        }

        return s1;
    }

    int memcmp(const void * s1, const void * s2, size_t n)
    {
        auto s1_u8 = reinterpret_cast<const uint8_t *>(s1);
        auto s2_u8 = reinterpret_cast<const uint8_t *>(s2);

        while (n--)
        {
            if (*s1_u8 != *s2_u8)
            {
                return *s1_u8 - *s2_u8;
            }
        }

        return 0;
    }

    void * memchr(void * s, int c, size_t n)
    {
        auto uc = static_cast<unsigned char>(c);
        auto cp = reinterpret_cast<unsigned char *>(s);

        for (size_t i = 0; i < n; ++i)
        {
            if (cp[i] == uc)
            {
                return reinterpret_cast<void *>(cp + i);
            }
        }

        return nullptr;
    }

    void * memset(void * s, int c, size_t n)
    {
        auto s_u8 = reinterpret_cast<uint8_t *>(s);

        while (n--)
        {
            *s_u8 = c;
        }

        return s;
    }

    size_t strlen(const char * s)
    {
        std::size_t ret = 0;

        while (*s++)
        {
            ++ret;
        }

        return ret;
    }
}

const void * memchr(const void * s, int c, size_t n)
{
    return memchr(const_cast<void *>(s), c, n);
}
}

// vim: ft=cpp
