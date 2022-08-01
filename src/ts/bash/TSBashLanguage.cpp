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

#include "ts/bash/TSBashLanguage.hpp"

#include <cassert>

#include "ts/bash/TSBashSType.hpp"
#include "ts/TSTransformer.hpp"
#include "TreeBuilder.hpp"
#include "mtypes.hpp"
#include "tree.hpp"
#include "types.hpp"

using namespace tsbash;

extern "C" const TSLanguage *tree_sitter_bash(void);

TsBashLanguage::TsBashLanguage() : tsLanguage(*tree_sitter_bash())
{
    stypes = {
        { "separator", +TSBashSType::Separator },
        { "comment", +TSBashSType::Comment },

        { "program", +TSBashSType::Program },
    };

    types = {
    };
}

Type
TsBashLanguage::mapToken(int token) const
{ return static_cast<Type>(token); }

TreeBuilder
TsBashLanguage::parse(const std::string &contents,
                      const std::string &/*fileName*/, int tabWidth, bool debug,
                      cpp17::pmr::monolithic &mr) const
{
    TreeBuilder tb(mr);
    TSTransformer t(contents, tsLanguage, tb, stypes, types, badNodes, tabWidth,
                    debug);
    t.transform();

    return tb;
}

bool
TsBashLanguage::isTravellingNode(const Node */*x*/) const
{ return false; }

bool
TsBashLanguage::hasFixedStructure(const Node */*x*/) const
{ return false; }

bool
TsBashLanguage::canBeFlattened(const Node */*parent*/, const Node */*child*/,
                               int level) const
{ return (level >= 3); }

bool
TsBashLanguage::isUnmovable(const Node */*x*/) const
{ return false; }

bool
TsBashLanguage::isContainer(const Node */*x*/) const
{ return false; }

bool
TsBashLanguage::isDiffable(const Node *x) const
{ return Language::isDiffable(x); }

bool
TsBashLanguage::isStructural(const Node *x) const
{ return Language::isStructural(x); }

bool
TsBashLanguage::isEolContinuation(const Node */*x*/) const
{ return false; }

bool
TsBashLanguage::alwaysMatches(const Node *x) const
{ return (-x->stype == TSBashSType::Program); }

bool
TsBashLanguage::isPseudoParameter(const Node */*x*/) const
{ return false; }

bool
TsBashLanguage::shouldSplice(SType /*parent*/, const Node */*childNode*/) const
{ return false; }

bool
TsBashLanguage::isValueNode(SType /*stype*/) const
{ return false; }

bool
TsBashLanguage::isLayerBreak(SType /*parent*/, SType stype) const
{ return isValueNode(stype); }

bool
TsBashLanguage::shouldDropLeadingWS(SType /*stype*/) const
{ return false; }

bool
TsBashLanguage::isSatellite(SType stype) const
{ return (-stype == TSBashSType::Separator); }

MType
TsBashLanguage::classify(SType stype) const
{
    switch (-stype) {
        case TSBashSType::Comment:
            return MType::Comment;

        default:
            return MType::Other;
    }
}

const char *
TsBashLanguage::toString(SType stype) const
{
    switch (-stype) {
        case TSBashSType::None:      return "TSBashSType::None";

        case TSBashSType::Separator: return "TSBashSType::Separator";
        case TSBashSType::Comment:   return "TSBashSType::Comment";

        case TSBashSType::Program:   return "TSBashSType::Program";
    }

    assert(false && "Unhandled enumeration item");
    return "<UNKNOWN>";
}
