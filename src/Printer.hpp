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

#ifndef ZOGRASCOPE__PRINTER_HPP__
#define ZOGRASCOPE__PRINTER_HPP__

#include <iosfwd>
#include <string>
#include <vector>

class Language;
class Node;
class TimeReport;

struct Header
{
    std::string left;
    std::string right;
};

class Printer
{
public:
    Printer(const Node &left, const Node &right, const Language &lang,
            std::ostream &os);
    Printer(const Node &left, std::vector<std::string> &&leftAnnots,
            const Node &right, std::vector<std::string> &&rightAnnots,
            const Language &lang, std::ostream &os);

public:
    void addHeader(Header header);
    void print(TimeReport &tr);

private:
    const Node &left, &right;
    std::vector<std::string> leftAnnots, rightAnnots;
    const Language &lang;
    std::ostream &os;
    std::vector<Header> headers;
};

#endif // ZOGRASCOPE__PRINTER_HPP__
