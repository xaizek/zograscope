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

#ifndef ZOGRASCOPE_TS_LUA_TSLUALANGUAGE_HPP_
#define ZOGRASCOPE_TS_LUA_TSLUALANGUAGE_HPP_

#include <unordered_map>
#include <unordered_set>

#include "Language.hpp"

struct TSLanguage;

// Lua-specific routines.
class TsLuaLanguage : public Language
{
public:
    // Initializes Lua-specific data.
    TsLuaLanguage();

public:
    // Maps language-specific token to an element of Type enumeration.
    virtual Type mapToken(int token) const override;
    // Parses source file into a tree.
    virtual TreeBuilder parse(const std::string &contents,
                              const std::string &fileName,
                              int tabWidth,
                              bool debug,
                              cpp17::pmr::monolithic &mr) const override;

    // Checks whether node doesn't have fixed position within a tree and can
    // move between internal nodes as long as post-order of leafs is preserved.
    virtual bool isTravellingNode(const Node *x) const override;
    // Checks whether the node enforces fixed structure (fixed number of
    // children at particular places).
    virtual bool hasFixedStructure(const Node *x) const override;
    // Checks whether a node can be flattened on a specific level of flattening.
    virtual bool canBeFlattened(const Node *parent, const Node *child,
                                int level) const override;
    // Checks whether a node should be considered for a move.
    virtual bool isUnmovable(const Node *x) const override;
    // Checks whether a node is a container.
    virtual bool isContainer(const Node *x) const override;
    // Checks whether spelling of a node can be diffed.
    virtual bool isDiffable(const Node *x) const override;
    // Checks whether a node represents a structural (auxiliary, like braces)
    // token.
    virtual bool isStructural(const Node *x) const override;
    // Checks whether a node represents a token used to declare line
    // continuation.
    virtual bool isEolContinuation(const Node *x) const override;
    // Checks whether a node always matches another node with the same stype.
    virtual bool alwaysMatches(const Node *x) const override;
    // Checks whether parameter node (as reported by `classify()`) represents a
    // true parameter and not something like "no argument list".
    virtual bool isPseudoParameter(const Node *x) const override;
    // Checks whether child node needs to be replaced in its parent with its
    // children.
    virtual bool shouldSplice(SType parent,
                              const Node *childNode) const override;
    // Checks whether the type corresponds to a value node.
    virtual bool isValueNode(SType stype) const override;
    // Checks whether this node with its descendants should be placed one level
    // deeper.
    virtual bool isLayerBreak(SType parent, SType stype) const override;
    // Checks whether leading space in spelling of nodes of this kind should be
    // skipped for the purposes of comparison.
    virtual bool shouldDropLeadingWS(SType stype) const override;
    // Checks whether nodes of this kind are secondary for comparison.
    virtual bool isSatellite(SType stype) const override;
    // Maps language-specific stype to generic mtype.
    virtual MType classify(SType stype) const override;
    // Stringifies value of SType enumeration.
    virtual const char * toString(SType stype) const override;

private:
    const TSLanguage &tsLanguage;                  // Language description.
    std::unordered_map<std::string, SType> stypes; // Maps nodes to STypes.
    std::unordered_map<std::string, Type> types;   // Maps nodes to Types.
    std::unordered_set<std::string> badNodes;      // Lists nodes to ignore.
};

#endif // ZOGRASCOPE_TS_LUA_TSLUALANGUAGE_HPP_
