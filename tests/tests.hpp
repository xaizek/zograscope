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

#ifndef ZOGRASCOPE_TESTS__TESTS_HPP__
#define ZOGRASCOPE_TESTS__TESTS_HPP__

#include <cstdint>
#include <cstdlib>

#include <functional>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

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

// Temporary directory in RAII-style.
class TempDir
{
public:
    // Makes temporary directory, which is removed in destructor.
    explicit TempDir(const std::string &prefix);

    // Make sure temporary directory is deleted only once.
    TempDir(const TempDir &rhs) = delete;
    TempDir & operator=(const TempDir &rhs) = delete;

    // Removes temporary directory and all its content, if it still exists.
    ~TempDir();

public:
    // Provides implicit conversion to a directory path string.
    operator std::string() const
    { return path; }

    // Explicit conversion to a directory path string.
    const std::string & str() const
    { return path; }

private:
    std::string path; // Path to the temporary directory.
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

// Parses Lua source into a tree.
Tree parseLua(const std::string &str);

// Finds the first node of specified type which has a matching value of its
// label (or any label if `label` is an empty string).
const Node * findNode(const Tree &tree, Type type,
                      const std::string &label = {});

// Finds the first node that matches the predicate.
const Node * findNode(const Tree &tree, std::function<bool(const Node *)> pred,
                      bool skipLastLayer = false);

int countLeaves(const Node &root, State state);

int countInternal(const Node &root, SType stype, State state);

// Diffs two trees and prints result into a normalized string.
std::string compareAndPrint(Tree &&original, Tree &&updated,
                            bool skipRefine = false);

// Strips whitespace and drops empty lines.
std::string normalizeText(const std::string &s);

// Compares two C sources with expectation being embedded in them in form of
// trailing `/// <expectation>` markers.  Returns difference report.
std::string diffC(const std::string &left, const std::string &right,
                  bool skipRefine);
// This is a wrapper that makes reported failure point be somewhere in the test.
#define diffC(left, right, skipRefine) do { \
        std::string difference = diffC((left), (right), (skipRefine)); \
        CHECK(difference.empty()); \
        reportDiffFailure(difference); \
    } while (false)

// Compares two Make sources with expectation being embedded in them in form of
// trailing `## <expectation>` markers.  Returns difference report.
std::string diffMake(const std::string &left, const std::string &right);
// This is a wrapper that makes reported failure point be somewhere in the test.
#define diffMake(left, right) do { \
        std::string difference = diffMake((left), (right)); \
        CHECK(difference.empty()); \
        reportDiffFailure(difference); \
    } while (false)

// Compares two C++ sources with expectation being embedded in them in form of
// trailing `/// <expectation>` markers.  Returns difference report.
std::string diffSrcmlCxx(const std::string &left, const std::string &right);
// This is a wrapper that makes reported failure point be somewhere in the test.
#define diffSrcmlCxx(left, right) do { \
        std::string difference = diffSrcmlCxx((left), (right)); \
        CHECK(difference.empty()); \
        reportDiffFailure(difference); \
    } while (false)

// Compares two Lua sources with expectation being embedded in them in form of
// trailing `--- <expectation>` markers.  Returns difference report.
std::string diffTsLua(const std::string &left, const std::string &right);
// This is a wrapper that makes reported failure point be somewhere in the test.
#define diffTsLua(left, right) do { \
        std::string difference = diffTsLua((left), (right)); \
        CHECK(difference.empty()); \
        reportDiffFailure(difference); \
    } while (false)

// Prints report.  This function is needed to make our custom output appear
// after Catch's failure report.
void reportDiffFailure(const std::string &report);

// Creates a file with specified contents.
void makeFile(const std::string &path, const std::vector<std::string> &lines);

#endif // ZOGRASCOPE_TESTS__TESTS_HPP__
