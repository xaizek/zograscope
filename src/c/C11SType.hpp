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

#ifndef ZOGRASCOPE__C__C11STYPE_HPP__
#define ZOGRASCOPE__C__C11STYPE_HPP__

#include <cstdint>

enum class SType : std::uint8_t;

// The namespace is necessary to avoid ODR violation for negation operator.
namespace c11stypes {

// C-specific STypes.
enum class C11SType : std::uint8_t
{
    None,
    TranslationUnit,
    Declaration,
    FunctionDeclaration,
    FunctionDefinition,
    Comment,
    Directive,
    LineGlue,
    Macro,
    CompoundStatement,
    Separator,
    Punctuation,
    Statements,
    ExprStatement,
    IfStmt,
    IfCond,
    IfThen,
    IfElse,
    WhileStmt,
    DoWhileStmt,
    WhileCond,
    ForStmt,
    LabelStmt,
    ForHead,
    Expression,
    AnyExpression,
    Declarator,
    Initializer,
    InitializerList,
    Specifiers,
    WithInitializer,
    WithoutInitializer,
    InitializerElement,
    SwitchStmt,
    GotoStmt,
    ContinueStmt,
    BreakStmt,
    ReturnValueStmt,
    ReturnNothingStmt,
    ArgumentList,
    Argument,
    ParameterList,
    Parameter,
    CallExpr,
    AssignmentExpr,
    ConditionExpr,
    ComparisonExpr,
    AdditiveExpr,
    PointerDecl,
    DirectDeclarator,
    SizeOf,
    TemporaryContainer,
    MemberAccess,
    Bundle,
    BundleComma,
};

// "Conversion operator": C11SType -> SType.
constexpr SType
operator+(C11SType stype)
{
    return static_cast<SType>(stype);
}

// "Conversion operator": SType -> C11SType.
constexpr C11SType
operator-(SType stype)
{
    return static_cast<C11SType>(stype);
}

}

#endif // ZOGRASCOPE__C__C11STYPE_HPP__
