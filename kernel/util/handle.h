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

#include <user/types.h>

#include "integer_types.h"
#include "intrusive_ptr.h"

namespace kernel::scheduler
{
class process;
}

namespace kernel
{
struct type_id
{
};

template<typename T>
type_id type_id_of;

struct handle_token_tag
{
};
using handle_token_t = tagged_integer_type<std::uintptr_t, handle_token_tag>;

class handle : public util::intrusive_ptrable<handle>
{
public:
    using dtor_t = void (*)(void *);

    handle(type_id * type, void * payload, dtor_t dtor, rose::syscall::permissions perms);
    ~handle();

    template<typename T>
    bool is_a() const
    {
        return _type == &type_id_of<T>;
    }

    template<typename T>
    T * get_as() const
    {
        return static_cast<T *>(_payload);
    }

    bool has_permissions(rose::syscall::permissions perms) const
    {
        return (std::to_underlying(perms) & std::to_underlying(_perms)) == std::to_underlying(perms);
    }

private:
    const rose::syscall::permissions _perms;
    type_id * _type;
    void * _payload;
    dtor_t _dtor;
};

template<typename T>
util::intrusive_ptr<handle> create_handle(
    T * payload,
    rose::syscall::permissions perms = rose::syscall::permissions::all)
{
    return util::make_intrusive<handle>(
        &type_id_of<T>, payload, +[](void *) {}, perms);
}

template<typename T>
util::intrusive_ptr<handle> create_handle(
    util::intrusive_ptr<T> payload,
    rose::syscall::permissions perms = rose::syscall::permissions::all)
{
    return util::make_intrusive<handle>(
        &type_id_of<T>,
        payload.release(util::keep_count),
        +[](void * payload) { util::intrusive_ptr(static_cast<T *>(payload)).release(util::drop_count); },
        perms);
}

inline struct kernel_caps_t
{
} kernel_caps;
}
