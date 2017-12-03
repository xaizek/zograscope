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

#ifndef ZOGRASCOPE_TESTS__TESTS_HPP__
#define ZOGRASCOPE_TESTS__TESTS_HPP__

#include <cstdint>

#include <ostream>
#include <sstream>
#include <string>

class Node;
class Tree;

enum class SType : std::uint8_t;
enum class State : std::uint8_t;
enum class Type : std::uint8_t;

/**
 * @brief Temporarily redirects specified stream into a string.
 */
class StreamCapture
{
public:
    /**
     * @brief Constructs instance that redirects @p os.
     *
     * @param os Stream to redirect.
     */
    StreamCapture(std::ostream &os) : os(os)
    {
        rdbuf = os.rdbuf();
        os.rdbuf(oss.rdbuf());
    }

    /**
     * @brief Restores original state of the stream.
     */
    ~StreamCapture()
    {
        os.rdbuf(rdbuf);
    }

public:
    /**
     * @brief Retrieves captured output collected so far.
     *
     * @returns String containing the output.
     */
    std::string get() const
    {
        return oss.str();
    }

private:
    std::ostream &os;       //!< Stream that is being redirected.
    std::ostringstream oss; //!< Temporary output buffer of the stream.
    std::streambuf *rdbuf;  //!< Original output buffer of the stream.
};

// Checks whether C source can be parsed or not.
bool cIsParsed(const std::string &str);

Tree makeTree(const std::string &str, bool coarse = false);

const Node * findNode(const Tree &tree, Type type,
                      const std::string &label = {});

int countLeaves(const Node &root, State state);

int countInternal(const Node &root, SType stype, State state);

void diffSources(const std::string &left, const std::string &right,
                 bool skipRefine);

#endif // ZOGRASCOPE_TESTS__TESTS_HPP__
