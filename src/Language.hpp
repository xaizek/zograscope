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

#ifndef ZOGRASCOPE__LANGUAGE_HPP__
#define ZOGRASCOPE__LANGUAGE_HPP__

#include <cstdint>

#include <memory>
#include <string>

namespace cpp17 {
    namespace pmr {
        class monolithic;
    }
}

class Node;
class TreeBuilder;

enum class SType : std::uint8_t;
enum class Type : std::uint8_t;

// Language-specific routines.
class Language
{
public:
    // Determines and creates language based on file name.  Non-empty `lang`
    // parameter forces specific language ("c" or "make").  Throws
    // `std::runtime_error` on incorrect language name.
    static std::unique_ptr<Language> create(const std::string &fileName,
                                            const std::string &lang = {});

public:
    // Virtual destructor for a base class.
    virtual ~Language() = default;

public:
    // Maps language-specific token to an element of Type enumeration.
    virtual Type mapToken(int token) const = 0;
    // Parses source file into a tree.
    virtual TreeBuilder parse(const std::string &contents,
                              const std::string &fileName,
                              bool debug,
                              cpp17::pmr::monolithic &mr) const = 0;

    // For children of nodes with fixed structure this checks whether this child
    // is first-class member of the structure or not (e.g., not punctuation).
    bool isPayloadOfFixed(const Node *x) const;
    // Checks whether node doesn't have fixed position within a tree and can
    // move between internal nodes as long as post-order of leafs is preserved.
    bool isTravellingNode(const Node *x) const;
    // Checks whether the node enforces fixed structure (fixed number of
    // children at particular places).
    bool hasFixedStructure(const Node *x) const;
    // Checks whether a node can be flattened on a specific level of flattening.
    bool canBeFlattened(const Node *parent, const Node *child, int level) const;
    // Checks whether children of the node can be considered for a move.
    bool hasMoveableItems(const Node *x) const;
    // Checks whether a node should be considered for a move.
    bool isUnmovable(const Node *x) const;
    // Checks whether a node is a container.
    bool isContainer(const Node *x) const;
    // Checks whether spelling of a node can be diffed.
    bool isDiffable(const Node *x) const;
    // Checks whether child node needs to be replaced in its parent with its
    // children.
    bool shouldSplice(SType parent, const Node *childNode) const;
    // Checks whether the type corresponds to a value node.
    bool isValueNode(SType stype) const;
    // Checks whether this node with its descendants should be placed one level
    // deeper.
    bool isLayerBreak(SType stype) const;
    // Stringifies value of SType enumeration.
    const char * toString(SType stype) const;
};

#endif // ZOGRASCOPE__LANGUAGE_HPP__
