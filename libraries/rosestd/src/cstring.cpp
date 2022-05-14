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

#include "../include/cstring"

__ROSESTD_OPEN

namespace
{
template<typename Repr>
void * __memcpy(void * s1, const void * s2, size_t n)
{
    auto s1_repr = reinterpret_cast<Repr *>(s1);
    auto s2_repr = reinterpret_cast<const Repr *>(s2);

    n /= sizeof(Repr);

    while (n--)
    {
        *s1_repr++ = *s2_repr++;
    }

    return s1;
}
}

#ifndef __ROSESTD_TESTING
extern "C"
{
#endif
    void * memcpy(void * s1, const void * s2, size_t n)
    {
        auto s1_uint = reinterpret_cast<uintptr_t>(s1);
        auto s2_uint = reinterpret_cast<uintptr_t>(s2);

        auto common = s1_uint | s2_uint | n;
        auto ffs = __builtin_ffsll(common);

        switch (ffs)
        {
            case 1:
                return __memcpy<uint8_t>(s1, s2, n);
            case 2:
                return __memcpy<uint16_t>(s1, s2, n);
            case 3:
                return __memcpy<uint32_t>(s1, s2, n);
            default:
                return __memcpy<uint64_t>(s1, s2, n);
        }
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

            ++s1_u8;
            ++s2_u8;
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

        auto end = s_u8 + n;

        while (s_u8 != end)
        {
            *s_u8++ = c;
        }

        return s;
    }

    size_t strlen(const char * s)
    {
        auto start = s;

        while (*s++)
        {
        }

        return s - start;
    }
#ifndef __ROSESTD_TESTING
}
#endif

const void * memchr(const void * s, int c, size_t n)
{
    return memchr(const_cast<void *>(s), c, n);
}

__ROSESTD_CLOSE
