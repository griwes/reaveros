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

#pragma once

#include "chained_allocator.h"

#include <atomic>

namespace kernel::util
{
inline struct adopt_t
{
} adopt;

inline struct keep_count_t
{
} keep_count;

inline struct drop_count_t
{
} drop_count;

template<typename T>
class intrusive_ptr
{
public:
    intrusive_ptr() = default;

    explicit intrusive_ptr(T * ptr) : _ptr(ptr)
    {
        _claim();
    }

    intrusive_ptr(T * ptr, adopt_t) : _ptr(ptr)
    {
    }

    intrusive_ptr(const intrusive_ptr<T> & other) : _ptr(other._ptr)
    {
        _claim();
    }

    intrusive_ptr(intrusive_ptr<T> && other) : _ptr(std::exchange(other._ptr, nullptr))
    {
    }

    intrusive_ptr & operator=(const intrusive_ptr & other)
    {
        _disown();
        _ptr = other._ptr;
        _claim();
        return *this;
    }

    intrusive_ptr & operator=(intrusive_ptr && other)
    {
        _disown();
        _ptr = std::exchange(other._ptr, nullptr);
        return *this;
    }

    ~intrusive_ptr()
    {
        _disown();
    }

    auto operator<=>(const intrusive_ptr<T> & other) const
    {
        return _ptr <=> other._ptr;
    }

    bool operator==(const intrusive_ptr<T> & other) const
    {
        return _ptr == other._ptr;
    }

    T & operator*() const
    {
        return *_ptr;
    }

    T * operator->() const
    {
        return _ptr;
    }

    T * get() const
    {
        return _ptr;
    }

    T * release(keep_count_t)
    {
        return std::exchange(_ptr, nullptr);
    }

    T * release(drop_count_t)
    {
        _ptr->intrusive_count.fetch_sub(1);
        return std::exchange(_ptr, nullptr);
    }

private:
    void _claim()
    {
        if (_ptr)
        {
            _ptr->intrusive_count.fetch_add(1);
        }
    }

    void _disown()
    {
        if (_ptr)
        {
            if (_ptr->intrusive_count.fetch_sub(1) == 1)
            {
                delete _ptr;
            }
        }
    }

    T * _ptr = nullptr;
};

template<typename T>
struct intrusive_ptr_preserve_count_traits
{
    using pointer = intrusive_ptr<T>;

    static auto create(T * pointer)
    {
        return intrusive_ptr<T>(pointer, adopt);
    };

    static auto unwrap(intrusive_ptr<T> pointer)
    {
        return pointer.release(keep_count);
    }
};

template<typename T>
struct intrusive_ptrable : chained_allocatable<T>
{
    std::atomic<std::size_t> intrusive_count{ 0 };
};

template<typename T, typename... Args>
intrusive_ptr<T> make_intrusive(Args &&... args)
{
    return intrusive_ptr(new T(std::forward<Args>(args)...));
}
}
