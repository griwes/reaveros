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

    handle(scheduler::process * owner, type_id * type, void * payload, dtor_t dtor);
    ~handle();

    handle_token_t get_token() const
    {
        return _token;
    }

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

private:
    handle_token_t _token;
    [[maybe_unused]] scheduler::process * _owner;
    type_id * _type;
    void * _payload;
    dtor_t _dtor;
};

template<typename T>
util::intrusive_ptr<handle> create_handle(scheduler::process * owner, T * payload)
{
    return util::make_intrusive<handle>(
        owner, &type_id_of<T>, payload, +[](void *) {});
}

template<typename T>
util::intrusive_ptr<handle> create_handle(scheduler::process * owner, util::intrusive_ptr<T> payload)
{
    return util::make_intrusive<handle>(
        owner,
        &type_id_of<T>,
        payload.release(util::keep_count),
        +[](void * payload) { util::intrusive_ptr(static_cast<T *>(payload)).release(util::drop_count); });
}

inline struct kernel_caps_t
{
} kernel_caps;
}
