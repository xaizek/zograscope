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

#ifndef ZOGRASCOPE__LANGUAGE_HPP__
#define ZOGRASCOPE__LANGUAGE_HPP__

#include <cstdint>

#include <memory>
#include <string>

namespace cpp17 {
    namespace pmr {
        class monolithic;
    }
}

class TreeBuilder;

enum class Type : std::uint8_t;

// Language-specific routines.
class Language
{
public:
    // Determines and creates language based on file name.
    static std::unique_ptr<Language> create(const std::string &fileName);

public:
    // Virtual destructor for a base class.
    virtual ~Language() = default;

public:
    // Maps language-specific token to an element of Type enumeration.
    virtual Type mapToken(int token) const = 0;
};

#endif // ZOGRASCOPE__LANGUAGE_HPP__
