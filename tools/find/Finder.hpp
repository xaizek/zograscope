// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE__TOOLS__FIND__FINDER_HPP__
#define ZOGRASCOPE__TOOLS__FIND__FINDER_HPP__

#include <boost/filesystem/path.hpp>

#include <deque>
#include <string>

class Args;
class Matcher;
class TimeReport;

// Processes files and looks for matches in them.
class Finder
{
public:
    // Parses arguments and records for future use.
    Finder(const Args &args, TimeReport &tr);
    // To destruct `matchers` field with complete type.
    ~Finder();

public:
    // Processes either single file or recursively discovered files in a
    // directory.
    bool find(const boost::filesystem::path &path);

public:
    // Processes single file.
    bool process(const std::string &path);
    // Prints report with statistics about results.
    void report();

private:
    const Args &args;             // Arguments of the tool.
    TimeReport &tr;               // Time reporter.
    std::deque<Matcher> matchers; // Storage of matchers.
};

#endif // ZOGRASCOPE__TOOLS__FIND__FINDER_HPP__
