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

#ifndef ZOGRASCOPE__MAKE__MAKESTYPE_HPP__
#define ZOGRASCOPE__MAKE__MAKESTYPE_HPP__

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

#endif // ZOGRASCOPE__MAKE__MAKESTYPE_HPP__
