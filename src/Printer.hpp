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

#ifndef ZOGRASCOPE__PRINTER_HPP__
#define ZOGRASCOPE__PRINTER_HPP__

#include <iosfwd>
#include <string>
#include <vector>

class Language;
class Node;
class TimeReport;

// Single table header for the printer.
struct Header
{
    std::string left;  // Left side value.
    std::string right; // Right side value.
};

// Prints two trees with nodes marked with comparison states along side each
// other with highlighting and alignment as textual table into a stream.
class Printer
{
public:
    // Remembers arguments for future use.  The references must be valid while
    // the object exists.
    Printer(const Node &left, const Node &right, const Language &lang,
            std::ostream &os);
    // Analogous constructor which additionally accepts annotations which are
    // prepended to source lines.
    Printer(const Node &left, std::vector<std::string> &&leftAnnots,
            const Node &right, std::vector<std::string> &&rightAnnots,
            const Language &lang, std::ostream &os);

public:
    // Adds table header.
    void addHeader(Header header);
    // Performs printing.
    void print(TimeReport &tr);

private:
    const Node &left, &right;                         // Tree roots.
    std::vector<std::string> leftAnnots, rightAnnots; // Annotations.
    const Language &lang;                             // Language of the trees.
    std::ostream &os;                                 // Output stream.
    std::vector<Header> headers;                      // Table headers.
};

#endif // ZOGRASCOPE__PRINTER_HPP__
