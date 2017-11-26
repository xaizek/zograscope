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

#ifndef ZOGRASCOPE__UTILS__STRINGS__HPP__
#define ZOGRASCOPE__UTILS__STRINGS__HPP__

#include <boost/utility/string_ref.hpp>

#include <vector>

class DiceString
{
public:
    DiceString(boost::string_ref s) : s(s)
    {
    }

public:
    float compare(DiceString &other);

    boost::string_ref str() const
    {
        return s;
    }

private:
    const std::vector<short> & getBigrams();

private:
    boost::string_ref s;
    std::vector<short> bigrams;
};

inline void
split(boost::string_ref str, char with, std::vector<boost::string_ref> &results)
{
    const char *next = &str.front();
    while (!str.empty()) {
        if (str.front() == with) {
            results.emplace_back(next, &str.front() - next);
            next = &str.front() + 1;
        }
        str.remove_prefix(1U);
    }
    results.emplace_back(next, &str.front() - next);
}

/**
 * @brief Splits string in a range-for loop friendly way.
 *
 * @param str String to split into substrings.
 * @param with Character to split at.
 *
 * @returns Array of results, empty on empty string.
 */
inline std::vector<boost::string_ref>
split(boost::string_ref str, char with)
{
    std::vector<boost::string_ref> results;
    split(str, with, results);
    return results;
}

#endif // ZOGRASCOPE__UTILS__STRINGS__HPP__
