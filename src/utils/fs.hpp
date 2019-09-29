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

#ifndef ZOGRASCOPE__UTILS__FS_HPP__
#define ZOGRASCOPE__UTILS__FS_HPP__

#include <string>

// Reads file into a string.  Throws `std::runtime_error` if `path` parameter
// specifies a directory or file reading has failed.
std::string readFile(const std::string &path);

#endif // ZOGRASCOPE__UTILS__FS_HPP__
