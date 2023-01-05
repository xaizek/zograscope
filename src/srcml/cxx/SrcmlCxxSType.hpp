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

#ifndef ZOGRASCOPE_SRCML_CXX_SRCMLCXXSTYPE_HPP_
#define ZOGRASCOPE_SRCML_CXX_SRCMLCXXSTYPE_HPP_

#include <cstdint>

enum class SType : std::uint8_t;

// The namespace is necessary to avoid ODR violation for negation operator.
namespace srcmlcxx {

// C++-specific STypes.
enum class SrcmlCxxSType : std::uint8_t
{
    None,
    Separator,
    Statements,

    Argument,
    Comment,
    CppEndif,
    CppLiteral,
    EnumDecl,
    Escape,
    ExprStmt,
    Incr,
    Macro,
    MacroList,
    RefQualifier,
    Unit,

    CppDefine,
    CppDirective,
    CppElif,
    CppElse,
    CppEmpty,
    CppError,
    CppFile,
    CppIf,
    CppIfdef,
    CppIfndef,
    CppInclude,
    CppLine,
    CppMacro,
    CppNumber,
    CppPragma,
    CppUndef,
    CppValue,
    CppWarning,

    Alignas,
    Alignof,
    ArgumentList,
    Asm,
    Assert,
    Attribute,
    Block,
    BlockContent,
    Break,
    Call,
    Capture,
    Case,
    Cast,
    Catch,
    Class,
    ClassDecl,
    Condition,
    Constructor,
    ConstructorDecl,
    Continue,
    Control,
    Decl,
    DeclStmt,
    Decltype,
    Default,
    Destructor,
    DestructorDecl,
    Do,
    Else,
    Elseif,
    EmptyStmt,
    Enum,
    Expr,
    Extern,
    For,
    Friend,
    Function,
    FunctionDecl,
    Goto,
    If,
    IfStmt,
    Index,
    Init,
    Label,
    Lambda,
    Literal,
    MemberInitList,
    MemberList,
    Modifier,
    Name,
    Namespace,
    Noexcept,
    Operator,
    Param,
    Parameter,
    ParameterList,
    Private,
    Protected,
    Public,
    Range,
    Return,
    Sizeof,
    Specifier,
    Struct,
    StructDecl,
    Super,
    SuperList,
    Switch,
    Template,
    Ternary,
    Then,
    Throw,
    Try,
    Type,
    Typedef,
    Typeid,
    Typename,
    Union,
    UnionDecl,
    Using,
    While,
};

// "Conversion operator": SrcmlCxxSType -> SType.
constexpr SType
operator+(SrcmlCxxSType stype)
{
    return static_cast<SType>(stype);
}

// "Conversion operator": SType -> SrcmlCxxSType.
constexpr SrcmlCxxSType
operator-(SType stype)
{
    return static_cast<SrcmlCxxSType>(stype);
}

}

#endif // ZOGRASCOPE_SRCML_CXX_SRCMLCXXSTYPE_HPP_
