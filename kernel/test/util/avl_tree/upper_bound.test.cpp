/*
 * Copyright © 2022 Michał 'Griwes' Dominiak
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

#include "../../../util/avl_tree.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <vector>

struct foo : kernel::util::treeable<foo>
{
    int id;
};

struct comp
{
    bool operator()(const foo & lhs, const foo & rhs) const
    {
        return lhs.id < rhs.id;
    }
};

int main()
{
    std::vector insert = { 17, 14, 19, 12, 11, 7, 2, 8, 9, 27, 29, 21, 22, 25, 24, 31, 32, 30 };
    auto iter = insert;

    kernel::util::avl_tree<foo, comp> tree;

    for (auto && i : insert)
    {
        auto f = std::make_unique<foo>();
        f->id = i;
        tree.insert(std::move(f));
    }

    std::map<int, int> expected_results = { { 15, 17 }, { 7, 8 }, { 28, 29 }, { 0, 2 }, { 17, 19 } };

    for (auto [k, v] : expected_results)
    {
        auto it = tree.upper_bound(foo{ .id = k });
        assert(it->id == v);
    }

    assert(tree.upper_bound(foo{ .id = 33 }) == tree.end());
}
