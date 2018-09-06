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

#include "DiffList.hpp"

#include <string>
#include <utility>
#include <vector>

#include "utils/fs.hpp"

DiffEntryFile::DiffEntryFile(std::string path)
    : title(path), path(std::move(path))
{
    contents = readFile(this->path);
}

DiffEntryFile::DiffEntryFile(std::string title, std::string path)
    : title(std::move(title)), path(std::move(path))
{
    contents = readFile(this->path);
}

DiffEntryFile::DiffEntryFile(std::string title, std::string path,
                             std::string contents)
    : title(std::move(title)), path(std::move(path)),
      contents(std::move(contents))
{ }

void
DiffList::add(DiffEntry entry)
{
    entries.push_back(std::move(entry));
}

bool
DiffList::empty() const
{
    return entries.empty();
}

const DiffEntry &
DiffList::getCurrent() const
{
    return entries[current];
}

const std::vector<DiffEntry> &
DiffList::getEntries() const
{
    return entries;
}

void
DiffList::nextEntry()
{
    if (current + 1 < entries.size()) {
        ++current;
    }
}

void
DiffList::previousEntry()
{
    if (current > 0U) {
        --current;
    }
}
