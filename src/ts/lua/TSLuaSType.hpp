// Copyright (C) 2021 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE__TS__LUA__TSLUASTYPE_HPP__
#define ZOGRASCOPE__TS__LUA__TSLUASTYPE_HPP__

#include <cstdint>

enum class SType : std::uint8_t;

// The namespace is necessary to avoid ODR violation for negation operator.
namespace tslua {

// Lua-specific STypes.
enum class TSLuaSType : std::uint8_t
{
    None,

    Separator,
    Comment,

    Program,

    BinaryExpression,
    ConditionExpression,
    Expression,
    ParenthesizedExpression,
    PrefixExpression,
    UnaryExpression,

    Table,
    ExpressionList,

    Statement,
    CallStatement,
    DoStatement,
    EmptyStatement,
    ForGenericStatement,
    ForNumericStatement,
    FunctionDefinitionStatement,
    GotoStatement,
    LabelStatement,
    LocalFunctionDefinitionStatement,
    RepeatStatement,
    ReturnStatement,
    WhileStatement,

    IfStatement,
    ElseClause,
    ElseifClause,

    FunctionBody,
    FunctionDefinition,
    FunctionDefinitionSignature,

    LocalVariableDeclaration,
    Variable,
    VariableAssignment,
    VariableList,

    Parameter,
    ParameterList,

    ArgumentList,
    Attribute,
    Block,
    Call,
    Field,
    FieldList,

    BinaryOperator,
    UnaryOperator,
};

// "Conversion operator": TSLuaSType -> SType.
constexpr SType
operator+(TSLuaSType stype)
{
    return static_cast<SType>(stype);
}

// "Conversion operator": SType -> TSLuaSType.
constexpr TSLuaSType
operator-(SType stype)
{
    return static_cast<TSLuaSType>(stype);
}

}

#endif // ZOGRASCOPE__TS__LUA__TSLUASTYPE_HPP__
