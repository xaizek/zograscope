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

#include "Language.hpp"

#include <cassert>
#include <cstdint>

#include <memory>
#include <stdexcept>
#include <string>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/path.hpp>

#include "c/C11Language.hpp"
#include "make/MakeLanguage.hpp"
#include "stypes.hpp"
#include "tree.hpp"

static std::string detectLanguage(const std::string &stem,
                                  const std::string &ext);

std::unique_ptr<Language>
Language::create(const std::string &fileName, const std::string &l)
{
    std::string lang = l;
    if (lang.empty()) {
        boost::filesystem::path path = fileName;

        using boost::algorithm::to_lower_copy;
        lang = detectLanguage(to_lower_copy(path.stem().string()),
                              to_lower_copy(path.extension().string()));
    }

    if (lang == "c") {
        return std::unique_ptr<C11Language>(new C11Language());
    }
    if (lang == "make") {
        return std::unique_ptr<MakeLanguage>(new MakeLanguage());
    }
    throw std::runtime_error("Unknown language: \"" + lang + '"');
}

// Determines language from normalized stem and extension of a file.
static std::string
detectLanguage(const std::string &stem, const std::string &ext)
{
    if (ext == ".c" || ext == ".h") {
        return "c";
    }

    using boost::algorithm::ends_with;
    if (ends_with(stem, "makefile") || ext == ".mk" || ext == ".mak") {
        return "make";
    }

    // Assume C by default.
    return "c";
}

bool
Language::isPayloadOfFixed(const Node *x) const
{
    return !isSatellite(x->stype)
        && !isTravellingNode(x);
}

bool
Language::isTravellingNode(const Node *x) const
{
    return (x->stype == SType::Directive || x->stype == SType::Comment);
}

bool
Language::hasFixedStructure(const Node *x) const
{
    return (x->stype == SType::ForHead);
}

bool
Language::canBeFlattened(const Node *, const Node *child, int level) const
{
    switch (level) {
        case 0:
            return (child->stype == SType::IfCond);

        case 1:
            return (child->stype == SType::ExprStatement);

        case 2:
            return (child->stype == SType::AnyExpression);

        default:
            return child->stype != SType::Declaration
                && child->stype != SType::ReturnValueStmt
                && child->stype != SType::CallExpr
                && child->stype != SType::Initializer
                && child->stype != SType::Parameter;
    }
}

bool
Language::hasMoveableItems(const Node *x) const
{
    // XXX: with current implementations of isUnmovable() and isContainer()
    //      (they are identical) this condition is always true.
    return (!isUnmovable(x) || isContainer(x));
}

bool
Language::isUnmovable(const Node *x) const
{
    return x->stype == SType::Statements
        || x->stype == SType::Bundle
        || x->stype == SType::BundleComma;
}

bool
Language::isContainer(const Node *x) const
{
    return x->stype == SType::Statements
        || x->stype == SType::Bundle
        || x->stype == SType::BundleComma;
}

bool
Language::isDiffable(const Node *x) const
{
    return (x->stype == SType::Comment || x->type == Type::StrConstants);
}

bool
Language::alwaysMatches(const Node *x) const
{
    return (x->stype == SType::TranslationUnit);
}

bool
Language::shouldSplice(SType parent, const Node *childNode) const
{
    SType child = childNode->stype;

    if (parent == SType::Statements && child == SType::Statements) {
        return true;
    }

    if (parent == SType::FunctionDefinition &&
        child == SType::CompoundStatement) {
        return true;
    }

    if (childNode->type == Type::Virtual &&
        child == SType::TemporaryContainer) {
        return true;
    }

    // Work around situation when addition of compound block to a statement
    // leads to the only statement that was there being marked as moved.
    if (parent == SType::IfThen || parent == SType::IfElse ||
        parent == SType::SwitchStmt || parent == SType::WhileStmt ||
        parent == SType::DoWhileStmt) {
        if (child == SType::CompoundStatement) {
            return true;
        }
    }

    if (parent == SType::IfStmt) {
        if (child == SType::IfThen) {
            return true;
        }
    }

    return false;
}

bool
Language::isValueNode(SType stype) const
{
    return stype == SType::FunctionDeclaration
        || stype == SType::IfCond
        || stype == SType::WhileCond;
}

bool
Language::isLayerBreak(SType stype) const
{
    switch (stype) {
        case SType::FunctionDeclaration:
        case SType::FunctionDefinition:
        case SType::InitializerElement:
        case SType::InitializerList:
        case SType::Initializer:
        case SType::Declaration:
        case SType::IfCond:
        case SType::WhileCond:
        case SType::CallExpr:
        case SType::AssignmentExpr:
        case SType::ExprStatement:
        case SType::AnyExpression:
        case SType::ReturnValueStmt:
        case SType::Parameter:
        case SType::ForHead:
            return true;

        default:
            return false;
    }
}

bool
Language::shouldDropLeadingWS(SType stype) const
{
    return (stype == SType::Comment);
}

bool
Language::isSatellite(SType stype) const
{
    return (stype == SType::Separator);
}

const char *
Language::toString(SType stype) const
{
    switch (stype) {
        case SType::None:                return "SType::None";
        case SType::TranslationUnit:     return "SType::TranslationUnit";
        case SType::Declaration:         return "SType::Declaration";
        case SType::FunctionDeclaration: return "SType::FunctionDeclaration";
        case SType::FunctionDefinition:  return "SType::FunctionDefinition";
        case SType::Directive:           return "SType::Directive";
        case SType::LineGlue:            return "SType::LineGlue";
        case SType::Comment:             return "SType::Comment";
        case SType::Macro:               return "SType::Macro";
        case SType::CompoundStatement:   return "SType::CompoundStatement";
        case SType::Separator:           return "SType::Separator";
        case SType::Punctuation:         return "SType::Punctuation";
        case SType::Statements:          return "SType::Statements";
        case SType::ExprStatement:       return "SType::ExprStatement";
        case SType::IfStmt:              return "SType::IfStmt";
        case SType::IfCond:              return "SType::IfCond";
        case SType::IfThen:              return "SType::IfThen";
        case SType::IfElse:              return "SType::IfElse";
        case SType::WhileStmt:           return "SType::WhileStmt";
        case SType::DoWhileStmt:         return "SType::DoWhileStmt";
        case SType::WhileCond:           return "SType::WhileCond";
        case SType::ForStmt:             return "SType::ForStmt";
        case SType::LabelStmt:           return "SType::LabelStmt";
        case SType::ForHead:             return "SType::ForHead";
        case SType::Expression:          return "SType::Expression";
        case SType::AnyExpression:       return "SType::AnyExpression";
        case SType::Declarator:          return "SType::Declarator";
        case SType::Initializer:         return "SType::Initializer";
        case SType::InitializerList:     return "SType::InitializerList";
        case SType::Specifiers:          return "SType::Specifiers";
        case SType::WithInitializer:     return "SType::WithInitializer";
        case SType::WithoutInitializer:  return "SType::WithoutInitializer";
        case SType::InitializerElement:  return "SType::InitializerElement";
        case SType::SwitchStmt:          return "SType::SwitchStmt";
        case SType::GotoStmt:            return "SType::GotoStmt";
        case SType::ContinueStmt:        return "SType::ContinueStmt";
        case SType::BreakStmt:           return "SType::BreakStmt";
        case SType::ReturnValueStmt:     return "SType::ReturnValueStmt";
        case SType::ReturnNothingStmt:   return "SType::ReturnNothingStmt";
        case SType::ArgumentList:        return "SType::ArgumentList";
        case SType::Argument:            return "SType::Argument";
        case SType::ParameterList:       return "SType::ParameterList";
        case SType::Parameter:           return "SType::Parameter";
        case SType::CallExpr:            return "SType::CallExpr";
        case SType::AssignmentExpr:      return "SType::AssignmentExpr";
        case SType::ConditionExpr:       return "SType::ConditionExpr";
        case SType::ComparisonExpr:      return "SType::ComparisonExpr";
        case SType::AdditiveExpr:        return "SType::AdditiveExpr";
        case SType::PointerDecl:         return "SType::PointerDecl";
        case SType::DirectDeclarator:    return "SType::DirectDeclarator";
        case SType::TemporaryContainer:  return "SType::TemporaryContainer";
        case SType::Bundle:              return "SType::Bundle";
        case SType::BundleComma:         return "SType::BundleComma";
    }

    assert(false && "Unhandled enumeration item");
    return "<UNKNOWN>";
}
