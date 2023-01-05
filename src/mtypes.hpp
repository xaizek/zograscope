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

#ifndef ZOGRASCOPE_MTYPES_HPP_
#define ZOGRASCOPE_MTYPES_HPP_

#include <cstdint>

#include <iosfwd>

// Meta-types used to classify language-specific STypes.
enum class MType : std::uint8_t
{
    Other,       // Everything else.
    Declaration, // Any sort of declaration.
    Statement,   // Statement.
    Function,    // Functions (their definitions only).
    Call,        // Function invocation.
    Parameter,   // Parameter in declaration of a function.
    Comment,     // Comments of any kind.
    Directive,   // Preprocessor-alike directives.
    Block,       // A container of statements.
};

// Prints mtype as a string.
std::ostream & operator<<(std::ostream &os, MType mtype);

// Checks whether nodes of the meta-type can nest.
bool canNest(MType mtype);

#endif // ZOGRASCOPE_MTYPES_HPP_
