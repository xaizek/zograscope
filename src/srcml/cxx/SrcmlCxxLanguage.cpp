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

#include "srcml/cxx/SrcmlCxxLanguage.hpp"

#include <cassert>

#include "srcml/cxx/SrcmlCxxSType.hpp"
#include "srcml/SrcmlTransformer.hpp"
#include "TreeBuilder.hpp"
#include "mtypes.hpp"
#include "tree.hpp"
#include "types.hpp"

using namespace srcmlcxx;

static void postProcessTree(PNode *node, TreeBuilder &tb,
                            const std::string &contents);
static void postProcessBlock(PNode *node, TreeBuilder &tb,
                             const std::string &contents);

SrcmlCxxLanguage::SrcmlCxxLanguage()
{
    map["separator"] = +SrcmlCxxSType::Separator;

    map["argument"]      = +SrcmlCxxSType::Argument;
    map["comment"]       = +SrcmlCxxSType::Comment;
    map["cpp:endif"]     = +SrcmlCxxSType::CppEndif;
    map["cpp:literal"]   = +SrcmlCxxSType::CppLiteral;
    map["enum_decl"]     = +SrcmlCxxSType::EnumDecl;
    map["escape"]        = +SrcmlCxxSType::Escape;
    map["expr_stmt"]     = +SrcmlCxxSType::ExprStmt;
    map["incr"]          = +SrcmlCxxSType::Incr;
    map["macro"]         = +SrcmlCxxSType::Macro;
    map["macro-list"]    = +SrcmlCxxSType::MacroList;
    map["ref_qualifier"] = +SrcmlCxxSType::RefQualifier;
    map["unit"]          = +SrcmlCxxSType::Unit;

    map["cpp:define"]    = +SrcmlCxxSType::CppDefine;
    map["cpp:directive"] = +SrcmlCxxSType::CppDirective;
    map["cpp:elif"]      = +SrcmlCxxSType::CppElif;
    map["cpp:else"]      = +SrcmlCxxSType::CppElse;
    map["cpp:empty"]     = +SrcmlCxxSType::CppEmpty;
    map["cpp:error"]     = +SrcmlCxxSType::CppError;
    map["cpp:file"]      = +SrcmlCxxSType::CppFile;
    map["cpp:if"]        = +SrcmlCxxSType::CppIf;
    map["cpp:ifdef"]     = +SrcmlCxxSType::CppIfdef;
    map["cpp:ifndef"]    = +SrcmlCxxSType::CppIfndef;
    map["cpp:include"]   = +SrcmlCxxSType::CppInclude;
    map["cpp:line"]      = +SrcmlCxxSType::CppLine;
    map["cpp:macro"]     = +SrcmlCxxSType::CppMacro;
    map["cpp:number"]    = +SrcmlCxxSType::CppNumber;
    map["cpp:pragma"]    = +SrcmlCxxSType::CppPragma;
    map["cpp:undef"]     = +SrcmlCxxSType::CppUndef;
    map["cpp:value"]     = +SrcmlCxxSType::CppValue;
    map["cpp:warning"]   = +SrcmlCxxSType::CppWarning;

    map["alignas"]          = +SrcmlCxxSType::Alignas;
    map["alignof"]          = +SrcmlCxxSType::Alignof;
    map["argument_list"]    = +SrcmlCxxSType::ArgumentList;
    map["asm"]              = +SrcmlCxxSType::Asm;
    map["assert"]           = +SrcmlCxxSType::Assert;
    map["attribute"]        = +SrcmlCxxSType::Attribute;
    map["block"]            = +SrcmlCxxSType::Block;
    map["break"]            = +SrcmlCxxSType::Break;
    map["call"]             = +SrcmlCxxSType::Call;
    map["capture"]          = +SrcmlCxxSType::Capture;
    map["case"]             = +SrcmlCxxSType::Case;
    map["cast"]             = +SrcmlCxxSType::Cast;
    map["catch"]            = +SrcmlCxxSType::Catch;
    map["class"]            = +SrcmlCxxSType::Class;
    map["class_decl"]       = +SrcmlCxxSType::ClassDecl;
    map["condition"]        = +SrcmlCxxSType::Condition;
    map["constructor"]      = +SrcmlCxxSType::Constructor;
    map["constructor_decl"] = +SrcmlCxxSType::ConstructorDecl;
    map["continue"]         = +SrcmlCxxSType::Continue;
    map["control"]          = +SrcmlCxxSType::Control;
    map["decl"]             = +SrcmlCxxSType::Decl;
    map["decl_stmt"]        = +SrcmlCxxSType::DeclStmt;
    map["decltype"]         = +SrcmlCxxSType::Decltype;
    map["default"]          = +SrcmlCxxSType::Default;
    map["destructor"]       = +SrcmlCxxSType::Destructor;
    map["destructor_decl"]  = +SrcmlCxxSType::DestructorDecl;
    map["do"]               = +SrcmlCxxSType::Do;
    map["else"]             = +SrcmlCxxSType::Else;
    map["elseif"]           = +SrcmlCxxSType::Elseif;
    map["empty_stmt"]       = +SrcmlCxxSType::EmptyStmt;
    map["enum"]             = +SrcmlCxxSType::Enum;
    map["expr"]             = +SrcmlCxxSType::Expr;
    map["extern"]           = +SrcmlCxxSType::Extern;
    map["for"]              = +SrcmlCxxSType::For;
    map["friend"]           = +SrcmlCxxSType::Friend;
    map["function"]         = +SrcmlCxxSType::Function;
    map["function_decl"]    = +SrcmlCxxSType::FunctionDecl;
    map["goto"]             = +SrcmlCxxSType::Goto;
    map["if"]               = +SrcmlCxxSType::If;
    map["index"]            = +SrcmlCxxSType::Index;
    map["init"]             = +SrcmlCxxSType::Init;
    map["label"]            = +SrcmlCxxSType::Label;
    map["lambda"]           = +SrcmlCxxSType::Lambda;
    map["literal"]          = +SrcmlCxxSType::Literal;
    map["member_init_list"] = +SrcmlCxxSType::MemberInitList;
    map["member_list"]      = +SrcmlCxxSType::MemberList;
    map["modifier"]         = +SrcmlCxxSType::Modifier;
    map["name"]             = +SrcmlCxxSType::Name;
    map["namespace"]        = +SrcmlCxxSType::Namespace;
    map["noexcept"]         = +SrcmlCxxSType::Noexcept;
    map["operator"]         = +SrcmlCxxSType::Operator;
    map["param"]            = +SrcmlCxxSType::Param;
    map["parameter"]        = +SrcmlCxxSType::Parameter;
    map["parameter_list"]   = +SrcmlCxxSType::ParameterList;
    map["private"]          = +SrcmlCxxSType::Private;
    map["protected"]        = +SrcmlCxxSType::Protected;
    map["public"]           = +SrcmlCxxSType::Public;
    map["range"]            = +SrcmlCxxSType::Range;
    map["return"]           = +SrcmlCxxSType::Return;
    map["sizeof"]           = +SrcmlCxxSType::Sizeof;
    map["specifier"]        = +SrcmlCxxSType::Specifier;
    map["struct"]           = +SrcmlCxxSType::Struct;
    map["struct_decl"]      = +SrcmlCxxSType::StructDecl;
    map["super"]            = +SrcmlCxxSType::Super;
    map["switch"]           = +SrcmlCxxSType::Switch;
    map["template"]         = +SrcmlCxxSType::Template;
    map["ternary"]          = +SrcmlCxxSType::Ternary;
    map["then"]             = +SrcmlCxxSType::Then;
    map["throw"]            = +SrcmlCxxSType::Throw;
    map["try"]              = +SrcmlCxxSType::Try;
    map["type"]             = +SrcmlCxxSType::Type;
    map["typedef"]          = +SrcmlCxxSType::Typedef;
    map["typeid"]           = +SrcmlCxxSType::Typeid;
    map["typename"]         = +SrcmlCxxSType::Typename;
    map["union"]            = +SrcmlCxxSType::Union;
    map["union_decl"]       = +SrcmlCxxSType::UnionDecl;
    map["using"]            = +SrcmlCxxSType::Using;
    map["while"]            = +SrcmlCxxSType::While;

    keywords.insert({
        "alignas", "alignof", "asm", "auto", "bool", "break", "case", "catch",
        "char", "char16_t", "char32_t", "class", "const", "constexpr",
        "const_cast", "continue", "decltype", "default", "delete", "do",
        "double", "dynamic_cast", "else", "enum", "explicit", "export",
        "extern", "false", "float", "for", "friend", "goto", "if", "inline",
        "int", "long", "mutable", "namespace", "new", "noexcept", "nullptr",
        "operator", "private", "protected", "public", "register",
        "reinterpret_cast", "return", "short", "signed", "sizeof", "static",
        "static_assert", "static_cast", "struct", "switch", "template", "this",
        "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
        "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
        "while",
    });
}

Type
SrcmlCxxLanguage::mapToken(int token) const
{
    return static_cast<Type>(token);
}

TreeBuilder
SrcmlCxxLanguage::parse(const std::string &contents,
                        const std::string &/*fileName*/, bool /*debug*/,
                        cpp17::pmr::monolithic &mr) const
{
    TreeBuilder tb(mr);
    SrcmlTransformer(contents, tb, "C++", map, keywords).transform();

    postProcessTree(tb.getRoot(), tb, contents);

    return tb;
}

// Rewrites tree to be more diff-friendly.
static void
postProcessTree(PNode *node, TreeBuilder &tb, const std::string &contents)
{
    if (node->stype == +SrcmlCxxSType::Block) {
        postProcessBlock(node, tb, contents);
    }

    for (PNode *child : node->children) {
        postProcessTree(child, tb, contents);
    }
}

// Rewrites block nodes to be more diff-friendly.
static void
postProcessBlock(PNode *node, TreeBuilder &tb, const std::string &/*contents*/)
{
    // Children: `{` statement* `}`.
    if (node->children.size() > 2) {
        PNode *stmts = tb.addNode();
        stmts->stype = +SrcmlCxxSType::Statements;
        stmts->children.assign(++node->children.cbegin(),
                               --node->children.cend());

        node->children.erase(++node->children.cbegin(),
                             --node->children.cend());
        node->children.insert(++node->children.cbegin(), stmts);
        return;
    }

    // Children: statement.
    if (node->children.size() == 1) {
        PNode *stmts = tb.addNode();
        stmts->stype = +SrcmlCxxSType::Statements;
        stmts->children = node->children;

        node->children.assign({ stmts });
    }
}

bool
SrcmlCxxLanguage::isTravellingNode(const Node */*x*/) const
{
    return false;
}

bool
SrcmlCxxLanguage::hasFixedStructure(const Node */*x*/) const
{
    return false;
}

bool
SrcmlCxxLanguage::canBeFlattened(const Node */*parent*/, const Node *child,
                                 int level) const
{
    switch (level) {
        case 0:
        case 1:
        case 2:
            return false;
        default:
            return -child->stype != SrcmlCxxSType::Call
                && -child->stype != SrcmlCxxSType::FunctionDecl
                && -child->stype != SrcmlCxxSType::DeclStmt
                && -child->stype != SrcmlCxxSType::Parameter
                && -child->stype != SrcmlCxxSType::EnumDecl;
    }
}

bool
SrcmlCxxLanguage::isUnmovable(const Node *x) const
{
    return (-x->stype == SrcmlCxxSType::Statements);
}

bool
SrcmlCxxLanguage::isContainer(const Node *x) const
{
    return (-x->stype == SrcmlCxxSType::Statements);
}

bool
SrcmlCxxLanguage::isDiffable(const Node *x) const
{
    return x->type == Type::Comments
        || x->type == Type::StrConstants;
}

bool
SrcmlCxxLanguage::alwaysMatches(const Node *x) const
{
    return (-x->stype == SrcmlCxxSType::Unit);
}

bool
SrcmlCxxLanguage::shouldSplice(SType parent, const Node *childNode) const
{
    SrcmlCxxSType child = -childNode->stype;
    if (child == SrcmlCxxSType::Statements) {
        if (-parent == SrcmlCxxSType::Struct ||
            -parent == SrcmlCxxSType::Class ||
            -parent == SrcmlCxxSType::Enum) {
            return true;
        }
    }
    if (child == SrcmlCxxSType::Block) {
        if (-parent == SrcmlCxxSType::Function ||
            -parent == SrcmlCxxSType::Constructor ||
            -parent == SrcmlCxxSType::Destructor ||
            -parent == SrcmlCxxSType::Struct ||
            -parent == SrcmlCxxSType::Class ||
            -parent == SrcmlCxxSType::Enum ||
            -parent == SrcmlCxxSType::Then ||
            -parent == SrcmlCxxSType::Else ||
            -parent == SrcmlCxxSType::For ||
            -parent == SrcmlCxxSType::While ||
            -parent == SrcmlCxxSType::Do ||
            -parent == SrcmlCxxSType::Switch) {
            return true;
        }
    }
    if (-parent == SrcmlCxxSType::If) {
        if (child == SrcmlCxxSType::Then || child == SrcmlCxxSType::Else) {
            return true;
        }
    }
    if (-parent == SrcmlCxxSType::Call &&
        child == SrcmlCxxSType::ArgumentList) {
        return true;
    }
    if (child == SrcmlCxxSType::ParameterList) {
        return true;
    }
    return false;
}

bool
SrcmlCxxLanguage::isValueNode(SType stype) const
{
    return (-stype == SrcmlCxxSType::Condition);
}

bool
SrcmlCxxLanguage::isLayerBreak(SType stype) const
{
    return -stype == SrcmlCxxSType::Call
        || -stype == SrcmlCxxSType::Constructor
        || -stype == SrcmlCxxSType::Destructor
        || -stype == SrcmlCxxSType::Function
        || -stype == SrcmlCxxSType::FunctionDecl
        || -stype == SrcmlCxxSType::DeclStmt
        || -stype == SrcmlCxxSType::ExprStmt
        || -stype == SrcmlCxxSType::Parameter
        || -stype == SrcmlCxxSType::EnumDecl
        || -stype == SrcmlCxxSType::Return
        || -stype == SrcmlCxxSType::Name
        || -stype == SrcmlCxxSType::Lambda
        || isValueNode(stype);
}

bool
SrcmlCxxLanguage::shouldDropLeadingWS(SType /*stype*/) const
{
    return false;
}

bool
SrcmlCxxLanguage::isSatellite(SType stype) const
{
    return (-stype == SrcmlCxxSType::Separator);
}

MType
SrcmlCxxLanguage::classify(SType stype) const
{
    switch (-stype) {
        case SrcmlCxxSType::FunctionDecl:
        case SrcmlCxxSType::DeclStmt:
        case SrcmlCxxSType::EnumDecl:
            return MType::Declaration;

        case SrcmlCxxSType::Function:
        case SrcmlCxxSType::Constructor:
        case SrcmlCxxSType::Destructor:
            return MType::Function;

        case SrcmlCxxSType::Comment:
            return MType::Comment;

        case SrcmlCxxSType::CppDefine:
        case SrcmlCxxSType::CppDirective:
        case SrcmlCxxSType::CppElif:
        case SrcmlCxxSType::CppElse:
        case SrcmlCxxSType::CppEmpty:
        case SrcmlCxxSType::CppError:
        case SrcmlCxxSType::CppFile:
        case SrcmlCxxSType::CppIf:
        case SrcmlCxxSType::CppIfdef:
        case SrcmlCxxSType::CppIfndef:
        case SrcmlCxxSType::CppInclude:
        case SrcmlCxxSType::CppLine:
        case SrcmlCxxSType::CppMacro:
        case SrcmlCxxSType::CppNumber:
        case SrcmlCxxSType::CppPragma:
        case SrcmlCxxSType::CppUndef:
        case SrcmlCxxSType::CppValue:
        case SrcmlCxxSType::CppWarning:
            return MType::Directive;

        default:
            return MType::Other;
    }
}

const char *
SrcmlCxxLanguage::toString(SType stype) const
{
    switch (-stype) {
        case SrcmlCxxSType::None:       return "SrcmlCxxSType::None";
        case SrcmlCxxSType::Separator:  return "SrcmlCxxSType::Separator";
        case SrcmlCxxSType::Statements: return "SrcmlCxxSType::Statements";

        case SrcmlCxxSType::Argument:     return "SrcmlCxxSType::Argument";
        case SrcmlCxxSType::Comment:      return "SrcmlCxxSType::Comment";
        case SrcmlCxxSType::CppEndif:     return "SrcmlCxxSType::CppEndif";
        case SrcmlCxxSType::CppLiteral:   return "SrcmlCxxSType::CppLiteral";
        case SrcmlCxxSType::EnumDecl:     return "SrcmlCxxSType::EnumDecl";
        case SrcmlCxxSType::Escape:       return "SrcmlCxxSType::Escape";
        case SrcmlCxxSType::ExprStmt:     return "SrcmlCxxSType::ExprStmt";
        case SrcmlCxxSType::Incr:         return "SrcmlCxxSType::Incr";
        case SrcmlCxxSType::Macro:        return "SrcmlCxxSType::Macro";
        case SrcmlCxxSType::MacroList:    return "SrcmlCxxSType::MacroList";
        case SrcmlCxxSType::RefQualifier: return "SrcmlCxxSType::RefQualifier";
        case SrcmlCxxSType::Unit:         return "SrcmlCxxSType::Unit";

        case SrcmlCxxSType::CppDefine:    return "SrcmlCxxSType::CppDefine";
        case SrcmlCxxSType::CppDirective: return "SrcmlCxxSType::CppDirective";
        case SrcmlCxxSType::CppElif:      return "SrcmlCxxSType::CppElif";
        case SrcmlCxxSType::CppElse:      return "SrcmlCxxSType::CppElse";
        case SrcmlCxxSType::CppEmpty:     return "SrcmlCxxSType::CppEmpty";
        case SrcmlCxxSType::CppError:     return "SrcmlCxxSType::CppError";
        case SrcmlCxxSType::CppFile:      return "SrcmlCxxSType::CppFile";
        case SrcmlCxxSType::CppIf:        return "SrcmlCxxSType::CppIf";
        case SrcmlCxxSType::CppIfdef:     return "SrcmlCxxSType::CppIfdef";
        case SrcmlCxxSType::CppIfndef:    return "SrcmlCxxSType::CppIfndef";
        case SrcmlCxxSType::CppInclude:   return "SrcmlCxxSType::CppInclude";
        case SrcmlCxxSType::CppLine:      return "SrcmlCxxSType::CppLine";
        case SrcmlCxxSType::CppMacro:     return "SrcmlCxxSType::CppMacro";
        case SrcmlCxxSType::CppNumber:    return "SrcmlCxxSType::CppNumber";
        case SrcmlCxxSType::CppPragma:    return "SrcmlCxxSType::CppPragma";
        case SrcmlCxxSType::CppUndef:     return "SrcmlCxxSType::CppUndef";
        case SrcmlCxxSType::CppValue:     return "SrcmlCxxSType::CppValue";
        case SrcmlCxxSType::CppWarning:   return "SrcmlCxxSType::CppWarning";

        case SrcmlCxxSType::Alignas:         return "SrcmlCxxSType::Alignas";
        case SrcmlCxxSType::Alignof:         return "SrcmlCxxSType::Alignof";
        case SrcmlCxxSType::ArgumentList:    return "SrcmlCxxSType::ArgumentList";
        case SrcmlCxxSType::Asm:             return "SrcmlCxxSType::Asm";
        case SrcmlCxxSType::Assert:          return "SrcmlCxxSType::Assert";
        case SrcmlCxxSType::Attribute:       return "SrcmlCxxSType::Attribute";
        case SrcmlCxxSType::Block:           return "SrcmlCxxSType::Block";
        case SrcmlCxxSType::Break:           return "SrcmlCxxSType::Break";
        case SrcmlCxxSType::Call:            return "SrcmlCxxSType::Call";
        case SrcmlCxxSType::Capture:         return "SrcmlCxxSType::Capture";
        case SrcmlCxxSType::Case:            return "SrcmlCxxSType::Case";
        case SrcmlCxxSType::Cast:            return "SrcmlCxxSType::Cast";
        case SrcmlCxxSType::Catch:           return "SrcmlCxxSType::Catch";
        case SrcmlCxxSType::Class:           return "SrcmlCxxSType::Class";
        case SrcmlCxxSType::ClassDecl:       return "SrcmlCxxSType::ClassDecl";
        case SrcmlCxxSType::Condition:       return "SrcmlCxxSType::Condition";
        case SrcmlCxxSType::Constructor:     return "SrcmlCxxSType::Constructor";
        case SrcmlCxxSType::ConstructorDecl: return "SrcmlCxxSType::ConstructorDecl";
        case SrcmlCxxSType::Continue:        return "SrcmlCxxSType::Continue";
        case SrcmlCxxSType::Control:         return "SrcmlCxxSType::Control";
        case SrcmlCxxSType::Decl:            return "SrcmlCxxSType::Decl";
        case SrcmlCxxSType::DeclStmt:        return "SrcmlCxxSType::DeclStmt";
        case SrcmlCxxSType::Decltype:        return "SrcmlCxxSType::Decltype";
        case SrcmlCxxSType::Default:         return "SrcmlCxxSType::Default";
        case SrcmlCxxSType::Destructor:      return "SrcmlCxxSType::Destructor";
        case SrcmlCxxSType::DestructorDecl:  return "SrcmlCxxSType::DestructorDecl";
        case SrcmlCxxSType::Do:              return "SrcmlCxxSType::Do";
        case SrcmlCxxSType::Else:            return "SrcmlCxxSType::Else";
        case SrcmlCxxSType::Elseif:          return "SrcmlCxxSType::Elseif";
        case SrcmlCxxSType::EmptyStmt:       return "SrcmlCxxSType::EmptyStmt";
        case SrcmlCxxSType::Enum:            return "SrcmlCxxSType::Enum";
        case SrcmlCxxSType::Expr:            return "SrcmlCxxSType::Expr";
        case SrcmlCxxSType::Extern:          return "SrcmlCxxSType::Extern";
        case SrcmlCxxSType::For:             return "SrcmlCxxSType::For";
        case SrcmlCxxSType::Friend:          return "SrcmlCxxSType::Friend";
        case SrcmlCxxSType::Function:        return "SrcmlCxxSType::Function";
        case SrcmlCxxSType::FunctionDecl:    return "SrcmlCxxSType::FunctionDecl";
        case SrcmlCxxSType::Goto:            return "SrcmlCxxSType::Goto";
        case SrcmlCxxSType::If:              return "SrcmlCxxSType::If";
        case SrcmlCxxSType::Index:           return "SrcmlCxxSType::Index";
        case SrcmlCxxSType::Init:            return "SrcmlCxxSType::Init";
        case SrcmlCxxSType::Label:           return "SrcmlCxxSType::Label";
        case SrcmlCxxSType::Lambda:          return "SrcmlCxxSType::Lambda";
        case SrcmlCxxSType::Literal:         return "SrcmlCxxSType::Literal";
        case SrcmlCxxSType::MemberInitList:  return "SrcmlCxxSType::MemberInitList";
        case SrcmlCxxSType::MemberList:      return "SrcmlCxxSType::MemberList";
        case SrcmlCxxSType::Modifier:        return "SrcmlCxxSType::Modifier";
        case SrcmlCxxSType::Name:            return "SrcmlCxxSType::Name";
        case SrcmlCxxSType::Namespace:       return "SrcmlCxxSType::Namespace";
        case SrcmlCxxSType::Noexcept:        return "SrcmlCxxSType::Noexcept";
        case SrcmlCxxSType::Operator:        return "SrcmlCxxSType::Operator";
        case SrcmlCxxSType::Param:           return "SrcmlCxxSType::Param";
        case SrcmlCxxSType::Parameter:       return "SrcmlCxxSType::Parameter";
        case SrcmlCxxSType::ParameterList:   return "SrcmlCxxSType::ParameterList";
        case SrcmlCxxSType::Private:         return "SrcmlCxxSType::Private";
        case SrcmlCxxSType::Protected:       return "SrcmlCxxSType::Protected";
        case SrcmlCxxSType::Public:          return "SrcmlCxxSType::Public";
        case SrcmlCxxSType::Range:           return "SrcmlCxxSType::Range";
        case SrcmlCxxSType::Return:          return "SrcmlCxxSType::Return";
        case SrcmlCxxSType::Sizeof:          return "SrcmlCxxSType::Sizeof";
        case SrcmlCxxSType::Specifier:       return "SrcmlCxxSType::Specifier";
        case SrcmlCxxSType::Struct:          return "SrcmlCxxSType::Struct";
        case SrcmlCxxSType::StructDecl:      return "SrcmlCxxSType::StructDecl";
        case SrcmlCxxSType::Super:           return "SrcmlCxxSType::Super";
        case SrcmlCxxSType::Switch:          return "SrcmlCxxSType::Switch";
        case SrcmlCxxSType::Template:        return "SrcmlCxxSType::Template";
        case SrcmlCxxSType::Ternary:         return "SrcmlCxxSType::Ternary";
        case SrcmlCxxSType::Then:            return "SrcmlCxxSType::Then";
        case SrcmlCxxSType::Throw:           return "SrcmlCxxSType::Throw";
        case SrcmlCxxSType::Try:             return "SrcmlCxxSType::Try";
        case SrcmlCxxSType::Type:            return "SrcmlCxxSType::Type";
        case SrcmlCxxSType::Typedef:         return "SrcmlCxxSType::Typedef";
        case SrcmlCxxSType::Typeid:          return "SrcmlCxxSType::Typeid";
        case SrcmlCxxSType::Typename:        return "SrcmlCxxSType::Typename";
        case SrcmlCxxSType::Union:           return "SrcmlCxxSType::Union";
        case SrcmlCxxSType::UnionDecl:       return "SrcmlCxxSType::UnionDecl";
        case SrcmlCxxSType::Using:           return "SrcmlCxxSType::Using";
        case SrcmlCxxSType::While:           return "SrcmlCxxSType::While";
    }

    assert(false && "Unhandled enumeration item");
    return "<UNKNOWN>";
}
