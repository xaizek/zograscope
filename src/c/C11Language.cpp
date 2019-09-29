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

#include "C11Language.hpp"

#include "c/C11SType.hpp"
#include "c/c11-parser.hpp"
#include "mtypes.hpp"
#include "tree.hpp"
#include "types.hpp"

using namespace c11stypes;

C11Language::C11Language() : map(NTOKENS)
{
    map[FUNCTION] = Type::Functions;

    map[TYPENAME] = Type::UserTypes;

    map[ID] = Type::Identifiers;

    map[BREAK]    = Type::Jumps;
    map[CONTINUE] = Type::Jumps;
    map[GOTO]     = Type::Jumps;

    map[_ALIGNAS]      = Type::Specifiers;
    map[EXTERN]        = Type::Specifiers;
    map[STATIC]        = Type::Specifiers;
    map[_THREAD_LOCAL] = Type::Specifiers;
    map[_ATOMIC]       = Type::Specifiers;
    map[AUTO]          = Type::Specifiers;
    map[REGISTER]      = Type::Specifiers;
    map[INLINE]        = Type::Specifiers;
    map[_NORETURN]     = Type::Specifiers;
    map[CONST]         = Type::Specifiers;
    map[VOLATILE]      = Type::Specifiers;
    map[RESTRICT]      = Type::Specifiers;
    map[TYPEDEF]       = Type::Specifiers;

    map[VOID]     = Type::Types;
    map[CHAR]     = Type::Types;
    map[SHORT]    = Type::Types;
    map[INT]      = Type::Types;
    map[LONG]     = Type::Types;
    map[FLOAT]    = Type::Types;
    map[DOUBLE]   = Type::Types;
    map[SIGNED]   = Type::Types;
    map[UNSIGNED] = Type::Types;
    map[_BOOL]    = Type::Types;
    map[_COMPLEX] = Type::Types;

    map['('] = Type::LeftBrackets;
    map['{'] = Type::LeftBrackets;
    map['['] = Type::LeftBrackets;

    map[')'] = Type::RightBrackets;
    map['}'] = Type::RightBrackets;
    map[']'] = Type::RightBrackets;

    map[LTE_OP] = Type::Comparisons;
    map[GTE_OP] = Type::Comparisons;
    map[EQ_OP]  = Type::Comparisons;
    map[NE_OP]  = Type::Comparisons;
    map['<']    = Type::Comparisons;
    map['>']    = Type::Comparisons;

    map[INC_OP] = Type::Operators;
    map[DEC_OP] = Type::Operators;
    map[LSH_OP] = Type::Operators;
    map[RSH_OP] = Type::Operators;
    map['&']    = Type::Operators;
    map['|']    = Type::Operators;
    map['^']    = Type::Operators;
    map['*']    = Type::Operators;
    map['/']    = Type::Operators;
    map['%']    = Type::Operators;
    map['+']    = Type::Operators;
    map['-']    = Type::Operators;
    map['~']    = Type::Operators;
    map['!']    = Type::Operators;

    map[AND_OP] = Type::LogicalOperators;
    map[OR_OP]  = Type::LogicalOperators;

    map['=']         = Type::Assignments;
    map[TIMESEQ_OP]  = Type::Assignments;
    map[DIVEQ_OP]    = Type::Assignments;
    map[MODEQ_OP]    = Type::Assignments;
    map[PLUSEQ_OP]   = Type::Assignments;
    map[MINUSEQ_OP]  = Type::Assignments;
    map[LSHIFTEQ_OP] = Type::Assignments;
    map[RSHIFTEQ_OP] = Type::Assignments;
    map[ANDEQ_OP]    = Type::Assignments;
    map[XOREQ_OP]    = Type::Assignments;
    map[OREQ_OP]     = Type::Assignments;

    map[DIRECTIVE] = Type::Directives;

    map[SLCOMMENT] = Type::Comments;
    map[MLCOMMENT] = Type::Comments;

    map[SLIT]    = Type::StrConstants;
    map[ICONST]  = Type::IntConstants;
    map[FCONST]  = Type::FPConstants;
    map[CHCONST] = Type::CharConstants;

    map[DEFAULT]        = Type::Keywords;
    map[RETURN]         = Type::Keywords;
    map[SIZEOF]         = Type::Keywords;
    map[_ALIGNOF]       = Type::Keywords;
    map[_GENERIC]       = Type::Keywords;
    map[DOTS]           = Type::Keywords;
    map[_STATIC_ASSERT] = Type::Keywords;
    map[IF]             = Type::Keywords;
    map[ELSE]           = Type::Keywords;
    map[SWITCH]         = Type::Keywords;
    map[WHILE]          = Type::Keywords;
    map[DO]             = Type::Keywords;
    map[FOR]            = Type::Keywords;
    map[CASE]           = Type::Keywords;
    map[STRUCT]         = Type::Keywords;
    map[UNION]          = Type::Keywords;
    map[ENUM]           = Type::Keywords;
    map[__ASM__]        = Type::Keywords;
    map[__VOLATILE__]   = Type::Keywords;

    map['?']    = Type::Other;
    map[':']    = Type::Other;
    map[';']    = Type::Other;
    map['.']    = Type::Other;
    map[',']    = Type::Other;
    map[ARR_OP] = Type::Other;
}

Type
C11Language::mapToken(int token) const
{
    return map[token];
}

TreeBuilder
C11Language::parse(const std::string &contents, const std::string &fileName,
                   bool debug, cpp17::pmr::monolithic &mr) const
{
    return c11_parse(contents, fileName, debug, mr);
}

bool
C11Language::isTravellingNode(const Node *x) const
{
    return -x->stype == C11SType::Directive
        || -x->stype == C11SType::Comment;
}

bool
C11Language::hasFixedStructure(const Node *x) const
{
    return (-x->stype == C11SType::ForHead);
}

bool
C11Language::canBeFlattened(const Node *, const Node *child, int level) const
{
    switch (level) {
        case 0:
            return (-child->stype == C11SType::IfCond);

        case 1:
            return (-child->stype == C11SType::ExprStatement);

        case 2:
            return (-child->stype == C11SType::AnyExpression);

        default:
            return -child->stype != C11SType::Declaration
                && -child->stype != C11SType::ReturnValueStmt
                && -child->stype != C11SType::CallExpr
                && -child->stype != C11SType::Parameter;
    }
}

bool
C11Language::isUnmovable(const Node *x) const
{
    return -x->stype == C11SType::Statements
        || -x->stype == C11SType::Bundle
        || -x->stype == C11SType::BundleComma;
}

bool
C11Language::isContainer(const Node *x) const
{
    return -x->stype == C11SType::Statements
        || -x->stype == C11SType::Bundle
        || -x->stype == C11SType::BundleComma;
}

bool
C11Language::isDiffable(const Node *x) const
{
    return -x->stype == C11SType::Comment
        || -x->stype == C11SType::Directive
        || Language::isDiffable(x);
}

bool
C11Language::isStructural(const Node *x) const
{
    return Language::isStructural(x)
        || x->label == ","
        || x->label == ";";
}

bool
C11Language::isEolContinuation(const Node *x) const
{
    return (-x->stype == C11SType::LineGlue);
}

bool
C11Language::alwaysMatches(const Node *x) const
{
    return (-x->stype == C11SType::TranslationUnit);
}

bool
C11Language::isPseudoParameter(const Node *x) const
{
    return (x->label == "void");
}

bool
C11Language::shouldSplice(SType parent, const Node *childNode) const
{
    C11SType child = -childNode->stype;

    if (-parent == C11SType::Statements && child == C11SType::Statements) {
        return true;
    }

    if (-parent == C11SType::FunctionDefinition &&
        child == C11SType::CompoundStatement) {
        return true;
    }

    if (childNode->type == Type::Virtual &&
        child == C11SType::TemporaryContainer) {
        return true;
    }

    if (-parent == C11SType::IfThen || -parent == C11SType::IfElse ||
        -parent == C11SType::SwitchStmt || -parent == C11SType::WhileStmt ||
        -parent == C11SType::DoWhileStmt || -parent == C11SType::ForStmt) {
        if (child == C11SType::CompoundStatement) {
            return true;
        }
    }

    if (-parent == C11SType::IfStmt) {
        if (child == C11SType::IfThen) {
            return true;
        }
    }

    return false;
}

bool
C11Language::isValueNode(SType stype) const
{
    return -stype == C11SType::FunctionDeclaration
        || -stype == C11SType::IfCond
        || -stype == C11SType::WhileCond;
}

bool
C11Language::isLayerBreak(SType /*parent*/, SType stype) const
{
    switch (-stype) {
        case C11SType::FunctionDefinition:
        case C11SType::InitializerElement:
        case C11SType::InitializerList:
        case C11SType::Initializer:
        case C11SType::Declaration:
        case C11SType::CallExpr:
        case C11SType::AssignmentExpr:
        case C11SType::ExprStatement:
        case C11SType::AnyExpression:
        case C11SType::ReturnValueStmt:
        case C11SType::Parameter:
        case C11SType::ForHead:
        case C11SType::MemberAccess:
        case C11SType::Argument:
            return true;

        default:
            return isValueNode(stype);
    }
}

bool
C11Language::shouldDropLeadingWS(SType stype) const
{
    return (-stype == C11SType::Comment);
}

bool
C11Language::isSatellite(SType stype) const
{
    return (-stype == C11SType::Separator);
}

MType
C11Language::classify(SType stype) const
{
    switch (-stype) {
        case C11SType::Declaration:
        case C11SType::FunctionDeclaration:
            return MType::Declaration;

        case C11SType::ExprStatement:
            return MType::Statement;

        case C11SType::FunctionDefinition:
            return MType::Function;

        case C11SType::CallExpr:
            return MType::Call;

        case C11SType::Parameter:
            return MType::Parameter;

        case C11SType::Comment:
            return MType::Comment;

        case C11SType::Directive:
            return MType::Directive;

        case C11SType::Statements:
            return MType::Block;

        default:
            return MType::Other;
    }
}

const char *
C11Language::toString(SType stype) const
{
    switch (-stype) {
        case C11SType::None:                return "C11SType::None";
        case C11SType::TranslationUnit:     return "C11SType::TranslationUnit";
        case C11SType::Declaration:         return "C11SType::Declaration";
        case C11SType::FunctionDeclaration: return "C11SType::FunctionDeclaration";
        case C11SType::FunctionDefinition:  return "C11SType::FunctionDefinition";
        case C11SType::Directive:           return "C11SType::Directive";
        case C11SType::LineGlue:            return "C11SType::LineGlue";
        case C11SType::Comment:             return "C11SType::Comment";
        case C11SType::Macro:               return "C11SType::Macro";
        case C11SType::CompoundStatement:   return "C11SType::CompoundStatement";
        case C11SType::Separator:           return "C11SType::Separator";
        case C11SType::Punctuation:         return "C11SType::Punctuation";
        case C11SType::Statements:          return "C11SType::Statements";
        case C11SType::ExprStatement:       return "C11SType::ExprStatement";
        case C11SType::IfStmt:              return "C11SType::IfStmt";
        case C11SType::IfCond:              return "C11SType::IfCond";
        case C11SType::IfThen:              return "C11SType::IfThen";
        case C11SType::IfElse:              return "C11SType::IfElse";
        case C11SType::WhileStmt:           return "C11SType::WhileStmt";
        case C11SType::DoWhileStmt:         return "C11SType::DoWhileStmt";
        case C11SType::WhileCond:           return "C11SType::WhileCond";
        case C11SType::ForStmt:             return "C11SType::ForStmt";
        case C11SType::LabelStmt:           return "C11SType::LabelStmt";
        case C11SType::ForHead:             return "C11SType::ForHead";
        case C11SType::Expression:          return "C11SType::Expression";
        case C11SType::AnyExpression:       return "C11SType::AnyExpression";
        case C11SType::Declarator:          return "C11SType::Declarator";
        case C11SType::Initializer:         return "C11SType::Initializer";
        case C11SType::InitializerList:     return "C11SType::InitializerList";
        case C11SType::Specifiers:          return "C11SType::Specifiers";
        case C11SType::WithInitializer:     return "C11SType::WithInitializer";
        case C11SType::WithoutInitializer:  return "C11SType::WithoutInitializer";
        case C11SType::InitializerElement:  return "C11SType::InitializerElement";
        case C11SType::SwitchStmt:          return "C11SType::SwitchStmt";
        case C11SType::GotoStmt:            return "C11SType::GotoStmt";
        case C11SType::ContinueStmt:        return "C11SType::ContinueStmt";
        case C11SType::BreakStmt:           return "C11SType::BreakStmt";
        case C11SType::ReturnValueStmt:     return "C11SType::ReturnValueStmt";
        case C11SType::ReturnNothingStmt:   return "C11SType::ReturnNothingStmt";
        case C11SType::ArgumentList:        return "C11SType::ArgumentList";
        case C11SType::Argument:            return "C11SType::Argument";
        case C11SType::ParameterList:       return "C11SType::ParameterList";
        case C11SType::Parameter:           return "C11SType::Parameter";
        case C11SType::CallExpr:            return "C11SType::CallExpr";
        case C11SType::AssignmentExpr:      return "C11SType::AssignmentExpr";
        case C11SType::ConditionExpr:       return "C11SType::ConditionExpr";
        case C11SType::ComparisonExpr:      return "C11SType::ComparisonExpr";
        case C11SType::AdditiveExpr:        return "C11SType::AdditiveExpr";
        case C11SType::PointerDecl:         return "C11SType::PointerDecl";
        case C11SType::DirectDeclarator:    return "C11SType::DirectDeclarator";
        case C11SType::SizeOf:              return "C11SType::SizeOf";
        case C11SType::TemporaryContainer:  return "C11SType::TemporaryContainer";
        case C11SType::MemberAccess:        return "C11SType::MemberAccess";
        case C11SType::Bundle:              return "C11SType::Bundle";
        case C11SType::BundleComma:         return "C11SType::BundleComma";
    }

    assert(false && "Unhandled enumeration item");
    return "<UNKNOWN>";
}
