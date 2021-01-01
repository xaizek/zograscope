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

#ifndef ZOGRASCOPE__TOOLING__FINDER_HPP__
#define ZOGRASCOPE__TOOLING__FINDER_HPP__

#include <deque>
#include <string>

#include "Grepper.hpp"
#include "Matcher.hpp"

class CommonArgs;
class Config;
class TimeReport;

// Processes files and looks for matches in them.
class Finder
{
public:
    // Parses arguments and records for future use.
    Finder(const CommonArgs &args,
           TimeReport &tr,
           const Config &config,
           bool countOnly);
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
    const CommonArgs &args;         // Arguments of a tool.
    TimeReport &tr;                 // Time reporter.
    const Config &config;           // Configuration.
    bool countOnly;                 // Only print statistics about results.
    std::vector<std::string> paths; // List of paths to process.
    std::deque<Matcher> matchers;   // Storage of matchers.
    Grepper grepper;                // Finder of consecutive tokens.
};

#endif // ZOGRASCOPE__TOOLING__FINDER_HPP__
