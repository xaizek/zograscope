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

#ifndef ZOGRASCOPE__STYPES_HPP__
#define ZOGRASCOPE__STYPES_HPP__

#include <cstdint>

enum class SType : std::uint8_t
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
    TemporaryContainer,
    Bundle,
    BundleComma,
};

#endif // ZOGRASCOPE__STYPES_HPP__
