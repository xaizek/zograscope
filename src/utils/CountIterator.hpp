// Copyright (C) 2018 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ZOGRASCOPE_UTILS_COUNTITERATOR_HPP_
#define ZOGRASCOPE_UTILS_COUNTITERATOR_HPP_

#include <iterator>

// Output iterator that just counts number of times it was incremented.
class CountIterator :
    public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
    CountIterator & operator*() { return *this; }
    CountIterator & operator++() { return *this; }
    CountIterator & operator++(int) { return *this; }

    template <typename T>
    CountIterator & operator=(const T &)
    {
        ++count;
        return *this;
    }

    int getCount() const { return count; }

private:
    int count = 0;
};

#endif // ZOGRASCOPE_UTILS_COUNTITERATOR_HPP_
