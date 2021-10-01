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

#include "../../../util/tree_heap.h"

#include <algorithm>
#include <cassert>
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
    std::vector insert = { 7, 3, 4, 1, 2, 9, 9, 8, 1 };
    auto pop = insert;
    std::sort(pop.begin(), pop.end());

    kernel::util::tree_heap<foo, comp> heap;

    for (auto && i : insert)
    {
        auto f = std::make_unique<foo>();
        f->id = i;
        heap.push(std::move(f));
    }

    for (auto && i : pop)
    {
        auto v = heap.pop();
        assert(v->id == i);
    }

    assert(!heap.peek());
}
