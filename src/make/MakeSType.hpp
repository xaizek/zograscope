// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE_MAKE_MAKESTYPE_HPP_
#define ZOGRASCOPE_MAKE_MAKESTYPE_HPP_

#include <cstdint>

enum class SType : std::uint8_t;

// The namespace is necessary to avoid ODR violation for negation operator.
namespace makestypes {

// Makefile-specific STypes.
enum class MakeSType : std::uint8_t
{
    None,
    LineGlue,
    Makefile,
    Statements,
    Separator,
    IfStmt,
    IfCond,
    MultilineAssignment,
    TemporaryContainer,
    Include,
    Directive,
    Comment,
    AssignmentExpr,
    CallExpr,
    ArgumentList,
    Argument,
    Rule,
    Recipe,
    Punctuation,
};

// "Conversion operator": MakeSType -> SType.
constexpr SType
operator+(MakeSType stype)
{
    return static_cast<SType>(stype);
}

// "Conversion operator": SType -> MakeSType.
constexpr MakeSType
operator-(SType stype)
{
    return static_cast<MakeSType>(stype);
}

}

#endif // ZOGRASCOPE_MAKE_MAKESTYPE_HPP_
