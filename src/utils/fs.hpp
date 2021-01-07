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

// Temporary file in RAII-style.
class TempFile
{
public:
    // Makes temporary file whose name is a mangled version of an input name.
    // The file is removed in destructor.
    explicit TempFile(const std::string &prefix);

    // Make sure temporary file is deleted only once.
    TempFile(const TempFile &rhs) = delete;
    TempFile & operator=(const TempFile &rhs) = delete;

    // Removes temporary file, if it still exists.
    ~TempFile();

public:
    // Provides implicit conversion to a file path string.
    operator std::string() const
    { return path; }

    // Explicit conversion to a file path string.
    const std::string & str() const
    { return path; }

private:
    std::string path; // Path to the temporary file.
};

// Reads file into a string.  Throws `std::runtime_error` if `path` parameter
// specifies a directory or file reading has failed.
std::string readFile(const std::string &path);

#endif // ZOGRASCOPE__UTILS__FS_HPP__
