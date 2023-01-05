// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE_UTILS_STRINGS_HPP_
#define ZOGRASCOPE_UTILS_STRINGS_HPP_

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
    const std::vector<unsigned short> & getBigrams();

private:
    boost::string_ref s;
    std::vector<unsigned short> bigrams;
};

// Splits string in two parts at the leftmost delimiter.  Returns a pair of
// empty strings on failure.
inline std::pair<std::string, std::string>
splitAt(const std::string &s, char delim)
{
    const std::string::size_type pos = s.find(delim);
    if (pos == std::string::npos) {
        return { };
    }

    return { s.substr(0, pos), s.substr(pos + 1U) };
}

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

// Normalizes line endings in the string by turning DOS line endings into UNIX
// line endings.
std::string && normalizeEols(std::string &&str);

// Goes over characters of the string and updates line and column accordingly.
void updatePosition(boost::string_ref str, int tabWidth, int &line, int &col);

#endif // ZOGRASCOPE_UTILS_STRINGS_HPP_
