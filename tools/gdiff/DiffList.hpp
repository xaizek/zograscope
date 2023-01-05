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

#ifndef ZOGRASCOPE_TOOLS_GDIFF_DIFFLIST_HPP_
#define ZOGRASCOPE_TOOLS_GDIFF_DIFFLIST_HPP_

#include <string>
#include <vector>

struct DiffEntryFile
{
    std::string title;
    std::string path;
    std::string contents;

    DiffEntryFile() = default;
    DiffEntryFile(std::string path);
    DiffEntryFile(std::string title, std::string path);
    DiffEntryFile(std::string title, std::string path, std::string contents);
};

struct DiffEntry
{
    DiffEntryFile original;
    DiffEntryFile updated;
};

class DiffList
{
public:
    void add(DiffEntry entry);

    bool empty() const;
    int getCount() const;
    int getPosition() const;

    const DiffEntry & getCurrent() const;
    const std::vector<DiffEntry> & getEntries() const;

    bool nextEntry();
    bool previousEntry();

private:
    std::vector<DiffEntry> entries;
    unsigned int current = 0U;
};

#endif // ZOGRASCOPE_TOOLS_GDIFF_DIFFLIST_HPP_
