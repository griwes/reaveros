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

#include "../memory/vas.h"
#include "../util/avl_tree.h"
#include "../util/handle.h"
#include "../util/intrusive_ptr.h"

namespace kernel::scheduler
{
class thread;

class process : public util::intrusive_ptrable<process>
{
public:
    process(std::unique_ptr<vm::vas> address_space);

    handle_token_t register_for_token(util::intrusive_ptr<handle>);
    util::intrusive_ptr<handle> get_handle(handle_token_t token) const;

    util::intrusive_ptr<thread> create_thread();

    vm::vas * get_vas()
    {
        return _address_space.get();
    }

private:
    struct _handle_store : util::treeable<_handle_store>
    {
        handle_token_t token;
        util::intrusive_ptr<handle> handle;
    };

    struct _handle_store_compare
    {
        bool operator()(const _handle_store & lhs, const _handle_store & rhs) const
        {
            return lhs.token.value() < rhs.token.value();
        }

        bool operator()(const _handle_store & lhs, handle_token_t token) const
        {
            return lhs.token.value() < token;
        }

        bool operator()(handle_token_t token, const _handle_store & rhs) const
        {
            return token.value() < rhs.token.value();
        }
    };

    mutable std::mutex _lock;
    std::unique_ptr<vm::vas> _address_space;
    util::avl_tree<_handle_store, _handle_store_compare> _handles;
};
}
