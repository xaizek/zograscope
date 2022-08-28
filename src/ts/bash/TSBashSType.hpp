// Copyright (C) 2022 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE__TS__BASH__TSBASHSTYPE_HPP__
#define ZOGRASCOPE__TS__BASH__TSBASHSTYPE_HPP__

#include <cstdint>

enum class SType : std::uint8_t;

// The namespace is necessary to avoid ODR violation for negation operator.
namespace tsbash {

// Bash-specific STypes.
enum class TSBashSType : std::uint8_t
{
    None,

    Separator,
    Comment,

    Program,

    Command,
    DeclarationCommand,
    NegatedCommand,
    TestCommand,
    UnsetCommand,

    Statement,
    CStyleForStatement,
    CaseStatement,
    CompoundStatement,
    ForStatement,
    IfStatement,
    WhileStatement,

    VariableAssignment,

    Expression,
    PrimaryExpression,
    RedirectedStatement,
    Array,
    BinaryExpression,
    CaseItem,
    CommandName,
    CommandSubstitution,
    Concatenation,
    DoGroup,
    ElifClause,
    ElseClause,
    Expansion,
    ExpansionFlags,
    FileRedirect,
    FunctionDefinition,
    HeredocBody,
    HeredocRedirect,
    HerestringRedirect,
    List,
    LineContinuation,
    ParenthesizedExpression,
    Pipeline,
    PostfixExpression,
    ProcessSubstitution,
    SimpleExpansion,
    String,
    StringContent,
    StringExpansion,
    Subscript,
    Subshell,
    TernaryExpression,
    UnaryExpression,
    Word,
    AnsiiCString,
    FileDescriptor,
    HeredocStart,
    RawString,
    Regex,
    SpecialVariableName,
    TestOperator,
    VariableName,
};

// "Conversion operator": TSBashSType -> SType.
constexpr SType
operator+(TSBashSType stype)
{
    return static_cast<SType>(stype);
}

// "Conversion operator": SType -> TSBashSType.
constexpr TSBashSType
operator-(SType stype)
{
    return static_cast<TSBashSType>(stype);
}

}

#endif // ZOGRASCOPE__TS__BASH__TSBASHSTYPE_HPP__
