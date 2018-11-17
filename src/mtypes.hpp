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

#ifndef ZOGRASCOPE__MTYPES_HPP__
#define ZOGRASCOPE__MTYPES_HPP__

#include <cstdint>

#include <iosfwd>

// Meta-types used to classify language-specific STypes.
enum class MType : std::uint8_t
{
    Other,       // Everything else.
    Declaration, // Any sort of declaration.
    Statement,   // Statement.
    Function,    // Functions (their definitions only).
    Parameter,   // Parameter in declaration of a function.
    Comment,     // Comments of any kind.
    Directive,   // Preprocessor-alike directives.
};

// Prints mtype as a string.
std::ostream & operator<<(std::ostream &os, MType mtype);

#endif // ZOGRASCOPE__MTYPES_HPP__
