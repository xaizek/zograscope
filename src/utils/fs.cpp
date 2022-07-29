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

#include "fs.hpp"

#include <boost/filesystem/operations.hpp>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "utils/strings.hpp"

namespace fs = boost::filesystem;

static const char AppPrefix[] = "zs-";

TempFile::TempFile(const std::string &prefix)
{
    fs::path baseName = prefix;
    std::string stem = baseName.stem().string();
    std::string extension = baseName.extension().string();

    path = (
       fs::temp_directory_path()
     / fs::unique_path(AppPrefix + stem + "-%%%%-%%%%" + extension)
    ).string();
}

TempFile::~TempFile()
{
    static_cast<void>(std::remove(path.c_str()));
}

std::string
readFile(const std::string &path)
{
    if (fs::is_directory(path)) {
        throw std::runtime_error("Not a regular file: " + path);
    }

    std::ifstream ifile(path);
    if (!ifile) {
        throw std::runtime_error("Can't open file: " + path);
    }

    return normalizeEols({ std::istreambuf_iterator<char>{ifile}, {} });
}
