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

#ifndef ZOGRASCOPE_TS_TSTRANSFORMER_HPP_
#define ZOGRASCOPE_TS_TSTRANSFORMER_HPP_

#include <cstdint>

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "tree_sitter/api.h"

class PNode;
class TreeBuilder;

enum class SType : std::uint8_t;
enum class Type : std::uint8_t;

// Uses tree-sitter to parse a file and transforms the result into PTree.
class TSTransformer
{
public:
    // Remembers parameters to use them later.  `contents`, `styles` and `types`
    // have to be lvalues.
    TSTransformer(const std::string &contents,
                  const TSLanguage &tsLanguage,
                  TreeBuilder &tb,
                  const std::unordered_map<std::string, SType> &stypes,
                  const std::unordered_map<std::string, Type> &types,
                  const std::unordered_set<std::string> &badNodes,
                  int tabWidth,
                  bool debug);

public:
    // Does all the work of transforming.
    void transform();

private:
    // Transforms a single node while propagating last node type to leafs
    // without type.
    PNode * visit(const TSNode &node, Type defType);
    // Transforms a leaf.
    void visitLeaf(SType stype, PNode *pnode, const TSNode &leaf, Type defType);
    // Determines type of a child of the specified node.
    Type determineType(const TSNode &node);

private:
    const std::string &contents;                          // Contents to parse.
    const TSLanguage &tsLanguage;                         // Language to use.
    TreeBuilder &tb;                                      // Result builder.
    const std::unordered_map<std::string, SType> &stypes; // Node type -> SType.
    const std::unordered_map<std::string, Type> &types;   // Node type -> Type.
    const std::unordered_set<std::string> &badNodes;      // Nodes to ignore.
    std::unordered_set<std::string> badSTypes;            // Missing stypes.
    std::unordered_set<std::string> badTypes;             // Missing types.
    int line;                                             // Current line.
    int col;                                              // Current column.
    uint32_t position;                                    // Parsing position.
    int tabWidth;                                         // Tabulation width.
    bool debug;                                           // Debugging state.
};

#endif // ZOGRASCOPE_TS_TSTRANSFORMER_HPP_
