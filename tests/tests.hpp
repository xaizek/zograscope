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
#include <cstdlib>

#include <functional>
#include <ostream>
#include <sstream>
#include <string>

#include <boost/filesystem/operations.hpp>

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

/**
 * @brief Temporary file in RAII-style.
 */
class TempFile
{
public:
    /**
     * @brief Makes temporary file, which is removed in destructor.
     *
     * @param prefix File name prefix.
     */
    explicit TempFile(const std::string &prefix)
    {
        namespace fs = boost::filesystem;

        path = (fs::temp_directory_path()
             /  fs::unique_path("dit-" + prefix + "-%%%%-%%%%")).string();
    }

    /**
     * @brief Removes temporary file, if it still exists.
     */
    ~TempFile()
    {
        static_cast<void>(std::remove(path.c_str()));
    }

public:
    /**
     * @brief Provides implicit convertion to a file path string.
     *
     * @returns The path.
     */
    operator std::string() const
    {
        return path;
    }

private:
    /**
     * @brief Path to the temporary file.
     */
    std::string path;
};

// Checks whether C source can be parsed or not.
bool cIsParsed(const std::string &str);

// Checks whether Make source can be parsed or not.
bool makeIsParsed(const std::string &str);

// Parses C source into a tree.
Tree parseC(const std::string &str, bool coarse = false);

// Parses Make source into a tree.
Tree parseMake(const std::string &str);

// Parses C++ source into a tree.
Tree parseCxx(const std::string &str);

// Finds the first node of specified type which has a matching value of its
// label (or any label if `label` is an empty string).
const Node * findNode(const Tree &tree, Type type,
                      const std::string &label = {});

// Finds the first node that matches the predicate.
const Node * findNode(const Tree &tree, std::function<bool(const Node *)> pred);

int countLeaves(const Node &root, State state);

int countInternal(const Node &root, SType stype, State state);

// Strips whitespace and drops empty lines.
std::string normalizeText(const std::string &s);

// Compares two C sources with expectation being embedded in them in form of
// trailing `/// <expectation>` markers.
void diffC(const std::string &left, const std::string &right, bool skipRefine);

// Compares two Make sources with expectation being embedded in them in form of
// trailing `## <expectation>` markers.
void diffMake(const std::string &left, const std::string &right);

// Compares two C++ sources with expectation being embedded in them in form of
// trailing `/// <expectation>` markers.
void diffSrcmlCxx(const std::string &left, const std::string &right);

#endif // ZOGRASCOPE_TESTS__TESTS_HPP__
