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
#include <sstream>
#include <stdexcept>
#include <string>

#include "utils/strings.hpp"

std::string
readFile(const std::string &path)
{
    if (boost::filesystem::is_directory(path)) {
        throw std::runtime_error("Not a regular file: " + path);
    }

    std::ifstream ifile(path);
    if (!ifile) {
        throw std::runtime_error("Can't open file: " + path);
    }

    std::ostringstream iss;
    iss << ifile.rdbuf();
    return normalizeEols(iss.str());
}
