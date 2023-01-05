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

#ifndef ZOGRASCOPE_TOOLING_FINDER_HPP_
#define ZOGRASCOPE_TOOLING_FINDER_HPP_

#include <deque>
#include <string>

#include "Grepper.hpp"
#include "Matcher.hpp"

class Environment;
class Config;

// Processes files and looks for matches in them.
class Finder
{
public:
    // Parses arguments and records for future use.
    Finder(Environment &env, bool countOnly);
    // To destruct `matchers` field with complete type.
    ~Finder();

public:
    // Processes all specified paths.
    bool search();

private:
    // Processes single file.
    bool process(const std::string &path);
    // Prints report with statistics about results.
    void report();

private:
    Environment &env;               // Source of environment data.
    bool countOnly;                 // Only print statistics about results.
    std::vector<std::string> paths; // List of paths to process.
    std::deque<Matcher> matchers;   // Storage of matchers.
    Grepper grepper;                // Finder of consecutive tokens.
};

#endif // ZOGRASCOPE_TOOLING_FINDER_HPP_
