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

    bool operator()(const foo & lhs, int rhs) const
    {
        return lhs.id < rhs;
    }

    bool operator()(int lhs, const foo & rhs) const
    {
        return lhs < rhs.id;
    }
};

struct testcase
{
    std::vector<int> insert;
    std::vector<int> erase;
};

int main()
{
    testcase testcases[] = { { .insert = { 106134984, 266496256, 253006166, 254023220, 254118652, 255126344 },
                               .erase = { 266496256, 254118652 } },
                             { .insert = { 108652276, 254041048, 259291740, 243721704, 243970556, 245414700 },
                               .erase = { 254041048 } } };

    for (auto && [insert, erase] : testcases)
    {
        kernel::util::avl_tree<foo, comp> tree;

        for (auto && i : insert)
        {
            auto f = std::make_unique<foo>();
            f->id = i;
            auto result = tree.insert(std::move(f));
            assert(result.second);

            auto check = tree.check_invariants();
            assert(check.correct);
        }

        auto check = tree.check_invariants();
        assert(check.correct);

        for (auto && i : erase)
        {
            assert(tree.find(i) != tree.end());

            auto it = tree.erase(i);
            assert(it == tree.end() || it->id > i);

            auto check = tree.check_invariants();
            assert(check.correct);

            assert(tree.find(i) == tree.end());
        }
    }
}
