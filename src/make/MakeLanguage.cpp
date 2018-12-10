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

#include "MakeLanguage.hpp"

#include <cassert>

#include "make/MakeSType.hpp"
#include "make/make-parser.hpp"
#include "mtypes.hpp"
#include "tree.hpp"
#include "types.hpp"

using namespace makestypes;

MakeLanguage::MakeLanguage() : map()
{
    map[COMMENT] = Type::Comments;
    map[ASSIGN_OP] = Type::Assignments;
    map[CALL_PREFIX] = Type::LeftBrackets;
    map[CALL_SUFFIX] = Type::RightBrackets;
    map['('] = Type::LeftBrackets;
    map[')'] = Type::RightBrackets;
    map[':'] = Type::Operators;

    map[VAR] = Type::UserTypes;

    map[OVERRIDE] = Type::Keywords;
    map[EXPORT] = Type::Keywords;
    map[UNEXPORT] = Type::Keywords;

    map[IFDEF] = Type::Directives;
    map[IFNDEF] = Type::Directives;
    map[IFEQ] = Type::Directives;
    map[IFNEQ] = Type::Directives;
    map[ELSE] = Type::Directives;
    map[ENDIF] = Type::Directives;

    map[DEFINE] = Type::Directives;
    map[ENDEF] = Type::Directives;
    map[UNDEFINE] = Type::Directives;

    map[INCLUDE] = Type::Directives;

    map[SLIT] = Type::StrConstants;
}

Type
MakeLanguage::mapToken(int token) const
{
    return map[token];
}

TreeBuilder
MakeLanguage::parse(const std::string &contents, const std::string &fileName,
                    bool debug, cpp17::pmr::monolithic &mr) const
{
    return make_parse(contents, fileName, debug, mr);
}

bool
MakeLanguage::isTravellingNode(const Node */*x*/) const
{
    return false;
}

bool
MakeLanguage::hasFixedStructure(const Node */*x*/) const
{
    return false;
}

bool
MakeLanguage::canBeFlattened(const Node */*parent*/, const Node *child,
                             int level) const
{
    switch (level) {
        case 0:
        case 1:
        case 2:
            return false;
        default:
            return -child->stype != MakeSType::CallExpr;
    }
}

bool
MakeLanguage::isUnmovable(const Node *x) const
{
    return (-x->stype == MakeSType::Statements);
}

bool
MakeLanguage::isContainer(const Node *x) const
{
    return (-x->stype == MakeSType::Statements);
}

bool
MakeLanguage::isDiffable(const Node *x) const
{
    return -x->stype == MakeSType::Comment
        || Language::isDiffable(x);
}

bool
MakeLanguage::isStructural(const Node *x) const
{
    return Language::isStructural(x);
}

bool
MakeLanguage::isEolContinuation(const Node *x) const
{
    return (-x->stype == MakeSType::LineGlue);
}

bool
MakeLanguage::alwaysMatches(const Node *x) const
{
    return (-x->stype == MakeSType::Makefile);
}

bool
MakeLanguage::shouldSplice(SType parent, const Node *childNode) const
{
    MakeSType child = -childNode->stype;

    if (-parent == MakeSType::Statements && child == MakeSType::Statements) {
        return true;
    }

    if (childNode->type == Type::Virtual &&
        child == MakeSType::TemporaryContainer) {
        return true;
    }

    return false;
}

bool
MakeLanguage::isValueNode(SType stype) const
{
    return (-stype == MakeSType::IfCond);
}

bool
MakeLanguage::isLayerBreak(SType /*parent*/, SType stype) const
{
    return -stype == MakeSType::CallExpr
        || -stype == MakeSType::AssignmentExpr
        || -stype == MakeSType::Rule
        || isValueNode(stype);
}

bool
MakeLanguage::shouldDropLeadingWS(SType /*stype*/) const
{
    return false;
}

bool
MakeLanguage::isSatellite(SType stype) const
{
    return (-stype == MakeSType::Separator);
}

MType
MakeLanguage::classify(SType stype) const
{
    switch (-stype) {
        case MakeSType::Comment:
            return MType::Comment;

        case MakeSType::Directive:
        case MakeSType::Include:
            return MType::Directive;

        default:
            return MType::Other;
    }
}

const char *
MakeLanguage::toString(SType stype) const
{
    switch (-stype) {
        case MakeSType::None:                return "MakeSType::None";
        case MakeSType::LineGlue:            return "MakeSType::LineGlue";
        case MakeSType::Makefile:            return "MakeSType::Makefile";
        case MakeSType::Statements:          return "MakeSType::Statements";
        case MakeSType::Separator:           return "MakeSType::Separator";
        case MakeSType::IfStmt:              return "MakeSType::IfStmt";
        case MakeSType::IfCond:              return "MakeSType::IfCond";
        case MakeSType::MultilineAssignment: return "MakeSType::MultilineAssignment";
        case MakeSType::TemporaryContainer:  return "MakeSType::TemporaryContainer";
        case MakeSType::Include:             return "MakeSType::Include";
        case MakeSType::Directive:           return "MakeSType::Directive";
        case MakeSType::Comment:             return "MakeSType::Comment";
        case MakeSType::AssignmentExpr:      return "MakeSType::AssignmentExpr";
        case MakeSType::CallExpr:            return "MakeSType::CallExpr";
        case MakeSType::ArgumentList:        return "MakeSType::ArgumentList";
        case MakeSType::Argument:            return "MakeSType::Argument";
        case MakeSType::Rule:                return "MakeSType::Rule";
        case MakeSType::Punctuation:         return "MakeSType::Punctuation";
    }

    assert(false && "Unhandled enumeration item");
    return "<UNKNOWN>";
}
