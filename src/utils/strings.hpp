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

#ifndef UTILS__STRINGS__HPP__
#define UTILS__STRINGS__HPP__

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
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
    class string_ref : public boost::string_ref
    {
    public:
        string_ref(const char *begin, const char *end)
            : boost::string_ref(begin, end - begin)
        {
        }
    };

    if (str.empty()) {
        return {};
    }

    std::vector<string_ref> results;
    boost::split(results, str, boost::is_from_range(with, with));
    return { results.cbegin(), results.cend() };
}

#endif // UTILS__STRINGS__HPP__
