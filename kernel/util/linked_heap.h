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

#include "chained_allocator.h"

#include <climits>
#include <memory>

namespace kernel::util
{
template<typename T>
struct linked_heapable : chained_allocatable<T>
{
    T * parent = nullptr;
};

template<typename T, typename Comparator>
class linked_heap
{
public:
    ~linked_heap()
    {
        while (peek())
        {
            pop();
        }
    }

    void push(std::unique_ptr<T> element_uptr)
    {
        if (!_top)
        {
            _top = element_uptr.release();
            ++_size;
            return;
        }

        auto parent = _parent_of_leftmost_empty();
        auto element = element_uptr.release();

        element->parent = parent;
        element->prev = nullptr;
        element->next = nullptr;

        if (parent->prev)
        {
            parent->next = element;
        }
        else
        {
            parent->prev = element;
        }

        while (element != _top && _comp(*element, *parent))
        {
            _swap(parent, element);

            parent = element->parent;
        }

        ++_size;
    }

    std::unique_ptr<T> pop()
    {
        switch (_size)
        {
            case 0:
                return {};

            case 1:
                --_size;
                return std::unique_ptr<T>{ std::exchange(_top, nullptr) };

            case 2:
            {
                auto top = _top;

                _top = _top->prev;
                _top->parent = nullptr;

                --_size;
                return std::unique_ptr<T>{ top };
            }

            case 3:
            {
                auto top = _top;

                if (_comp(*top->prev, *top->next))
                {
                    _top = top->prev;
                    top->next->parent = _top;
                    _top->prev = top->next;
                }
                else
                {
                    _top = top->next;
                    top->prev->parent = _top;
                    _top->prev = top->prev;
                }

                _top->parent = nullptr;

                --_size;
                return std::unique_ptr<T>{ top };
            }
        }

        auto top = _top;
        auto element = _rightmost_full();

        _top = element;
        element->prev = top->prev;
        element->next = top->next;

        if (element == element->parent->prev)
        {
            element->parent->prev = nullptr;
        }
        else
        {
            element->parent->next = nullptr;
        }
        element->parent = nullptr;

        top->prev->parent = element;
        top->next->parent = element;

        while (true)
        {
            T * best = element;

            if (element->prev && _comp(*element->prev, *best))
            {
                best = element->prev;
            }

            if (element->next && _comp(*element->next, *best))
            {
                best = element->next;
            }

            if (best == element)
            {
                --_size;
                return std::unique_ptr<T>{ top };
            }

            _swap(element, best);
        }
    }

    const T * peek() const
    {
        return _top;
    }

private:
    T * _top = nullptr;
    std::size_t _size = 0;
    Comparator _comp;

    void _swap(T * parent, T * child)
    {
        auto pprev = parent->prev;
        auto pnext = parent->next;
        auto pparent = parent->parent;

        parent->prev = child->prev;
        parent->next = child->next;

        if (parent->prev)
        {
            parent->prev->parent = parent;
        }
        if (parent->next)
        {
            parent->next->parent = parent;
        }

        child->parent = parent->parent;
        parent->parent = child;

        if (pprev == child)
        {
            child->prev = parent;
            child->next = pnext;
            if (child->next)
            {
                child->next->parent = child;
            }
        }
        else
        {
            child->prev = pprev;
            if (child->prev)
            {
                child->prev->parent = child;
            }
            child->next = parent;
        }

        if (pparent)
        {
            if (pparent->prev == parent)
            {
                pparent->prev = child;
            }
            else
            {
                pparent->next = child;
            }
        }

        if (!child->parent)
        {
            _top = child;
        }
    }

    auto _get_nth(std::size_t nth) const
    {
        auto leading_zeroes = __builtin_clzl(nth);
        auto level = sizeof(std::size_t) * CHAR_BIT - leading_zeroes;

        // n is the level of the tree we are at
        std::size_t n = 1;
        // x is the numbers of elements in already discarded subtrees *to the left* (it's the offset of the
        // middle of the current subtree from 0)
        std::size_t x = 0;
        auto current = _top;

        auto level_elements = static_cast<std::size_t>(1) << (level - 1);
        nth -= level_elements - 1;

        // search until we get to the lowest level of the tree
        while (n < level)
        {
            // y is the slot index to the right of the middle of the current subtree
            std::size_t y = level_elements / (static_cast<std::size_t>(1) << n) + x;

            // if the middle is to the left of the index we are looking for, nth is in the right subtree
            if (y < nth)
            {
                // since we are moving right, we are discarding more elements on the left; update the offset
                // to the current subtree's middle (because that index is now always going to be to the left
                // of where we are searching through)
                x = y;
                current = current->next;
            }
            // otherwise, its in the left subtree
            else
            {
                current = current->prev;
            }

            n += 1;
        }

        return current;
    }

    auto _parent_of_leftmost_empty() const
    {
        return _get_nth((_size + 1) / 2);
    }

    auto _rightmost_full() const
    {
        return _get_nth(_size);
    }
};
}
