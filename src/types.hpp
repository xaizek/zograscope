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

#ifndef ZOGRASCOPE__TYPES_HPP__
#define ZOGRASCOPE__TYPES_HPP__

#include <cstdint>

#include <iosfwd>

enum class Type : std::uint8_t
{
    Virtual,

    // FUNCTION
    Functions,

    // TYPENAME
    UserTypes,

    // ID
    Identifiers,

    // BREAK
    // CONTINUE
    // GOTO
    Jumps,

    // _ALIGNAS
    // EXTERN
    // STATIC
    // _THREAD_LOCAL
    // _ATOMIC
    // AUTO
    // REGISTER
    // INLINE
    // _NORETURN
    // CONST
    // VOLATILE
    // RESTRICT
    Specifiers,

    // VOID
    // CHAR
    // SHORT
    // INT
    // LONG
    // FLOAT
    // DOUBLE
    // SIGNED
    // UNSIGNED
    // _BOOL
    // _COMPLEX
    Types,

    // '('
    // '{'
    // '['
    LeftBrackets,

    // ')'
    // '}'
    // ']'
    RightBrackets,

    // LTE_OP
    // GTE_OP
    // EQ_OP
    // NE_OP
    // '<'
    // '>'
    Comparisons,

    // INC_OP
    // DEC_OP
    // LSH_OP
    // RSH_OP
    // '&'
    // '|'
    // '^'
    // '*'
    // '/'
    // '%'
    // '+'
    // '-'
    // '~'
    // '!'
    Operators,

    // AND_OP
    // OR_OP
    LogicalOperators,

    // '='
    // TIMESEQ_OP
    // DIVEQ_OP
    // MODEQ_OP
    // PLUSEQ_OP
    // MINUSEQ_OP
    // LSHIFTEQ_OP
    // RSHIFTEQ_OP
    // ANDEQ_OP
    // XOREQ_OP
    // OREQ_OP
    Assignments,

    // DIRECTIVE
    Directives,

    // SLCOMMENT
    // MLCOMMENT
    Comments,

    // SLIT
    StrConstants,
    // ICONST
    IntConstants,
    // FCONST
    FPConstants,
    // CHCONST
    CharConstants,

    // This is a separator, all types below are not interchangeable.
    NonInterchangeable,

    // DEFAULT
    // RETURN
    // SIZEOF
    // _ALIGNOF
    // _GENERIC
    // DOTS
    // _STATIC_ASSERT
    // IF
    // ELSE
    // SWITCH
    // WHILE
    // DO
    // FOR
    // CASE
    // TYPEDEF
    // STRUCT
    // UNION
    // ENUM
    Keywords,

    // '?'
    // ':'
    // ';'
    // '.'
    // ','
    // ARR_OP
    Other,
};

std::ostream & operator<<(std::ostream &os, Type type);

Type canonizeType(Type type);

#endif // ZOGRASCOPE__TYPES_HPP__
