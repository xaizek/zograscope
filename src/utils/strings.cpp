// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#include "utils/strings.hpp"

#include <climits>

#include <iterator>
#include <limits>
#include <utility>

#include <boost/utility/string_ref.hpp>

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

float
DiceString::compare(DiceString &other)
{
    if (s.length() < 2U && other.s.length() < 2U) {
        return (s == other.s) ? 1.0f : 0.0f;
    }
    if (s.length() < 2U || other.s.length() < 2U) {
        return 0.0f;
    }

    const std::vector<short> &bigrams = getBigrams();
    const std::vector<short> &otherBigrams = other.getBigrams();
    const int common = std::set_intersection(bigrams.cbegin(), bigrams.cend(),
                                             otherBigrams.cbegin(),
                                             otherBigrams.cend(),
                                             CountIterator())
                      .getCount();

    return (2.0f*common)/(bigrams.size() + otherBigrams.size());
}

const std::vector<short> &
DiceString::getBigrams()
{
    if (!bigrams.empty() || s.length() < 2U) {
        // Either already computed or there is nothing to compute.
        return bigrams;
    }

    auto makeBigram = [](boost::string_ref s, std::size_t at) {
        return (static_cast<unsigned char>(s[at]) << CHAR_BIT)
              | static_cast<unsigned char>(s[at + 1U]);
    };

    // Using std::sort is fine for very small number of elements.
    if (s.length() < 10000) {
        bigrams.reserve(s.length() - 1U);
        for (std::size_t i = 0U; i < s.length() - 1U; ++i) {
            bigrams.push_back(makeBigram(s, i));
        }
        std::sort(bigrams.begin(), bigrams.end());
        bigrams.erase(std::unique(bigrams.begin(), bigrams.end()),
                      bigrams.end());

        return bigrams;
    }

    // But for string of tenths of thousands characters this works faster (exact
    // threshold yet to be determined).

    const int maxBigrams = std::numeric_limits<unsigned short>::max();
    thread_local std::vector<bool> present(maxBigrams);

    bigrams.reserve(s.length() - 1U);
    for (std::size_t i = 0U; i < s.length() - 1U; ++i) {
        present[makeBigram(s, i)] = true;
    }
    for (int i = 0; i < maxBigrams; ++i) {
        if (present[i]) {
            bigrams.push_back(i);
            present[i] = false;
        }
    }

    return bigrams;
}
