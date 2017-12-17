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
    return x->stype != SType::Separator
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
