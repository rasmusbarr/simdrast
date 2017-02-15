//
//  QuickSort.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-12-20.
//  This file contains modified code from the LLVM project's algorithm header. Original license:
/*
==============================================================================
LLVM Release License
==============================================================================
University of Illinois/NCSA
Open Source License

Copyright (c) 2003-2010 University of Illinois at Urbana-Champaign.
All rights reserved.

Developed by:

LLVM Team

University of Illinois at Urbana-Champaign

http://llvm.org

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal with
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimers.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimers in the
documentation and/or other materials provided with the distribution.

* Neither the names of the LLVM Team, University of Illinois at
Urbana-Champaign, nor the names of its contributors may be used to
endorse or promote products derived from this Software without specific
prior written permission.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
SOFTWARE.
*/

#ifndef SimdRast_QuickSort_h
#define SimdRast_QuickSort_h

#include "Config.h"
#include <algorithm>

namespace srast {

namespace llvm_algorithm_sort {

// stable, 2-3 compares, 0-2 swaps

template <class Compare, class ForwardIterator>
SRAST_FORCEINLINE unsigned sort3(ForwardIterator x, ForwardIterator y, ForwardIterator z, Compare c) {
    unsigned r = 0;
    if (!c(*y, *x))          // if x <= y
    {
        if (!c(*z, *y))      // if y <= z
            return r;            // x <= y && y <= z
                                   // x <= y && y > z
        std::swap(*y, *z);          // x <= z && y < z
        r = 1;
        if (c(*y, *x))       // if x > y
        {
            std::swap(*x, *y);      // x < y && y <= z
            r = 2;
        }
        return r;                // x <= y && y < z
    }
    if (c(*z, *y))           // x > y, if y > z
    {
        std::swap(*x, *z);          // x < y && y < z
        r = 1;
        return r;
    }
    std::swap(*x, *y);              // x > y && y <= z
    r = 1;                       // x < y && x <= z
    if (c(*z, *y))           // if y > z
    {
        std::swap(*y, *z);          // x <= y && y < z
        r = 2;
    }
    return r;
}                                  // x <= y && y <= z

// stable, 3-6 compares, 0-5 swaps

template <class Compare, class ForwardIterator>
SRAST_FORCEINLINE unsigned sort4(ForwardIterator x1, ForwardIterator x2, ForwardIterator x3, ForwardIterator x4, Compare c) {
    unsigned r = sort3<Compare>(x1, x2, x3, c);
    if (c(*x4, *x3))
    {
        std::swap(*x3, *x4);
        ++r;
        if (c(*x3, *x2))
        {
            std::swap(*x2, *x3);
            ++r;
            if (c(*x2, *x1))
            {
                std::swap(*x1, *x2);
                ++r;
            }
        }
    }
    return r;
}

// stable, 4-10 compares, 0-9 swaps

template <class Compare, class ForwardIterator>
SRAST_FORCEINLINE unsigned sort5(ForwardIterator x1, ForwardIterator x2, ForwardIterator x3, ForwardIterator x4, ForwardIterator x5, Compare c) {
    unsigned r = sort4<Compare>(x1, x2, x3, x4, c);
    if (c(*x5, *x4))
    {
        std::swap(*x4, *x5);
        ++r;
        if (c(*x4, *x3))
        {
            std::swap(*x3, *x4);
            ++r;
            if (c(*x3, *x2))
            {
                std::swap(*x2, *x3);
                ++r;
                if (c(*x2, *x1))
                {
                    std::swap(*x1, *x2);
                    ++r;
                }
            }
        }
    }
    return r;
}

template <class Compare, class RandomAccessIterator>
SRAST_FORCEINLINE void insertion_sort_3(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type value_type;
    RandomAccessIterator j = first+2;
    sort3<Compare>(first, first+1, j, comp);
    for (RandomAccessIterator i = j+1; i != last; ++i)
    {
        if (comp(*i, *j))
        {
            value_type t(std::move(*i));
            RandomAccessIterator k = j;
            j = i;
            do
            {
                *j = std::move(*k);
                j = k;
            } while (j != first && comp(t, *--k));
            *j = std::move(t);
        }
        j = i;
    }
}

template <class Compare, class RandomAccessIterator>
SRAST_FORCEINLINE bool insertion_sort_incomplete(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
    switch (last - first)
    {
    case 0:
    case 1:
        return true;
    case 2:
        if (comp(*--last, *first))
            std::swap(*first, *last);
        return true;
    case 3:
        sort3<Compare>(first, first+1, --last, comp);
        return true;
    case 4:
        sort4<Compare>(first, first+1, first+2, --last, comp);
        return true;
    case 5:
        sort5<Compare>(first, first+1, first+2, first+3, --last, comp);
        return true;
    }
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type value_type;
    RandomAccessIterator j = first+2;
    sort3<Compare>(first, first+1, j, comp);
    const unsigned limit = 8;
    unsigned count = 0;
    for (RandomAccessIterator i = j+1; i != last; ++i)
    {
        if (comp(*i, *j))
        {
            value_type t(std::move(*i));
            RandomAccessIterator k = j;
            j = i;
            do
            {
                *j = std::move(*k);
                j = k;
            } while (j != first && comp(t, *--k));
            *j = std::move(t);
            if (++count == limit)
                return ++i == last;
        }
        j = i;
    }
    return true;
}

template <class Compare, class RandomAccessIterator>
static void sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
    // Compare is known to be a reference type
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type difference_type;
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type value_type;
    const difference_type limit = std::is_trivially_copy_constructible<value_type>::value &&
                                    std::is_trivially_copy_assignable<value_type>::value ? 30 : 6;
    while (true)
    {
    restart:
        difference_type len = last - first;
        switch (len)
        {
        case 0:
        case 1:
            return;
        case 2:
            if (comp(*--last, *first))
                std::swap(*first, *last);
            return;
        case 3:
            sort3<Compare>(first, first+1, --last, comp);
            return;
        case 4:
            sort4<Compare>(first, first+1, first+2, --last, comp);
            return;
        case 5:
            sort5<Compare>(first, first+1, first+2, first+3, --last, comp);
            return;
        }
        if (len <= limit)
        {
            insertion_sort_3<Compare>(first, last, comp);
            return;
        }
        // len > 5
        RandomAccessIterator m = first;
        RandomAccessIterator lm1 = last;
        --lm1;
        unsigned n_swaps;
        {
        difference_type delta;
        if (len >= 1000)
        {
            delta = len/2;
            m += delta;
            delta /= 2;
            n_swaps = sort5<Compare>(first, first + delta, m, m+delta, lm1, comp);
        }
        else
        {
            delta = len/2;
            m += delta;
            n_swaps = sort3<Compare>(first, m, lm1, comp);
        }
        }
        // *m is median
        // partition [first, m) < *m and *m <= [m, last)
        // (this inhibits tossing elements equivalent to m around unnecessarily)
        RandomAccessIterator i = first;
        RandomAccessIterator j = lm1;
        // j points beyond range to be tested, *m is known to be <= *lm1
        // The search going up is known to be guarded but the search coming down isn't.
        // Prime the downward search with a guard.
        if (!comp(*i, *m))  // if *first == *m
        {
            // *first == *m, *first doesn't go in first part
            // manually guard downward moving j against i
            while (true)
            {
                if (i == --j)
                {
                    // *first == *m, *m <= all other elements
                    // Parition instead into [first, i) == *first and *first < [i, last)
                    ++i;  // first + 1
                    j = last;
                    if (!comp(*first, *--j))  // we need a guard if *first == *(last-1)
                    {
                        while (true)
                        {
                            if (i == j)
                                return;  // [first, last) all equivalent elements
                            if (comp(*first, *i))
                            {
                                std::swap(*i, *j);
                                ++n_swaps;
                                ++i;
                                break;
                            }
                            ++i;
                        }
                    }
                    // [first, i) == *first and *first < [j, last) and j == last - 1
                    if (i == j)
                        return;
                    while (true)
                    {
                        while (!comp(*first, *i))
                            ++i;
                        while (comp(*first, *--j))
                            ;
                        if (i >= j)
                            break;
                        std::swap(*i, *j);
                        ++n_swaps;
                        ++i;
                    }
                    // [first, i) == *first and *first < [i, last)
                    // The first part is sorted, sort the secod part
                    // sort<Compare>(i, last, comp);
                    first = i;
                    goto restart;
                }
                if (comp(*j, *m))
                {
                    std::swap(*i, *j);
                    ++n_swaps;
                    break;  // found guard for downward moving j, now use unguarded partition
                }
            }
        }
        // It is known that *i < *m
        ++i;
        // j points beyond range to be tested, *m is known to be <= *lm1
        // if not yet partitioned...
        if (i < j)
        {
            // known that *(i - 1) < *m
            // known that i <= m
            while (true)
            {
                // m still guards upward moving i
                while (comp(*i, *m))
                    ++i;
                // It is now known that a guard exists for downward moving j
                while (!comp(*--j, *m))
                    ;
                if (i > j)
                    break;
                std::swap(*i, *j);
                ++n_swaps;
                // It is known that m != j
                // If m just moved, follow it
                if (m == i)
                    m = j;
                ++i;
            }
        }
        // [first, i) < *m and *m <= [i, last)
        if (i != m && comp(*m, *i))
        {
            std::swap(*i, *m);
            ++n_swaps;
        }
        // [first, i) < *i and *i <= [i+1, last)
        // If we were given a perfect partition, see if insertion sort is quick...
        if (n_swaps == 0)
        {
            bool fs = insertion_sort_incomplete<Compare>(first, i, comp);
            if (insertion_sort_incomplete<Compare>(i+1, last, comp))
            {
                if (fs)
                    return;
                last = i;
                continue;
            }
            else
            {
                if (fs)
                {
                    first = ++i;
                    continue;
                }
            }
        }
        // sort smaller range with recursive call and larger with tail recursion elimination
        if (i - first < last - i)
        {
            sort<Compare>(first, i, comp);
            // sort<Compare>(i+1, last, comp);
            first = ++i;
        }
        else
        {
            sort<Compare>(i+1, last, comp);
            // sort<Compare>(first, i, comp);
            last = i;
        }
    }
}

}

template<class T, class C>
SRAST_FORCEINLINE void quickSort(T* first, T* last, C less) {
	llvm_algorithm_sort::sort(first, last, less);
}

template<class T>
struct LessComparator {
	SRAST_FORCEINLINE bool operator () (const T& first, const T& last) const {
		return first < last;
	}
};

template<class T>
SRAST_FORCEINLINE void quickSort(T* first, T* last) {
	return quickSort(first, last, LessComparator<T>());
}

}

#endif
