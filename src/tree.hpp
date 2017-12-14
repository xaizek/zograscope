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

#ifndef ZOGRASCOPE__TREE_HPP__
#define ZOGRASCOPE__TREE_HPP__

#include <cstdint>

#include <memory>
#include <string>
#include <vector>

#include "pmr/pmr_deque.hpp"
#include "pmr/pmr_vector.hpp"

#include "Language.hpp"
#include "types.hpp"

enum class State : std::uint8_t
{
    Unchanged,
    Deleted,
    Inserted,
    Updated
};

class PNode;
class SNode;

enum class SType : std::uint8_t;

struct Node
{
    using allocator_type = cpp17::pmr::polymorphic_allocator<cpp17::byte>;

    std::string label;
    std::string spelling;
    cpp17::pmr::vector<Node *> children;
    Node *relative = nullptr;
    Node *parent = nullptr;
    Node *next = nullptr;
    int valueChild = -1;
    int poID = -1; // post-order ID
    int line = 0;
    int col = 0;
    Type type : 8;
    SType stype : 8;
    State state : 8;
    bool satellite : 1;
    bool moved : 1;
    bool last : 1;
    bool leaf : 1;

    Node(allocator_type al = {})
        : children(al),
          type(Type::Virtual),
          stype(),
          state(State::Unchanged),
          satellite(false), moved(false), last(false), leaf(false)
    {
    }
    Node(const Node &rhs) = delete;
    Node(Node &&rhs) = default;
    Node(Node &&rhs, allocator_type al = {})
        : label(std::move(rhs.label)),
          spelling(std::move(rhs.spelling)),
          children(std::move(rhs.children), al),
          relative(rhs.relative),
          parent(rhs.parent),
          next(rhs.next),
          valueChild(rhs.valueChild),
          poID(rhs.poID),
          line(rhs.line),
          col(rhs.col),
          type(rhs.type),
          stype(rhs.stype),
          state(rhs.state),
          satellite(rhs.satellite),
          moved(rhs.moved),
          last(rhs.last),
          leaf(rhs.leaf)
    {
    }
    Node & operator=(const Node &rhs) = delete;
    Node & operator=(Node &&rhs) = default;

    bool hasValue() const
    {
        return (valueChild >= 0);
    }

    Node * getValue()
    {
        return (valueChild >= 0 ? children[valueChild] : nullptr);
    }

    const Node * getValue() const
    {
        return (valueChild >= 0 ? children[valueChild] : nullptr);
    }
};

class Tree
{
    using allocator_type = cpp17::pmr::polymorphic_allocator<cpp17::byte>;

public:
    Tree(allocator_type al = {}) : nodes(al)
    {
    }
    Tree(const Tree &rhs) = delete;
    Tree(Tree &&rhs) = default;
    Tree(std::unique_ptr<Language> lang, const std::string &contents,
         const PNode *node, allocator_type al = {});
    Tree(std::unique_ptr<Language> lang, const std::string &contents,
         const SNode *node, allocator_type al = {});

    Tree & operator=(const Tree &rhs) = delete;
    Tree & operator=(Tree &&rhs) = default;

public:
    Node * getRoot()
    {
        return root;
    }

    const Node * getRoot() const
    {
        return root;
    }

    Node & makeNode()
    {
        nodes.emplace_back();
        return nodes.back();
    }

    // Retrieves language associated with this tree.
    Language * getLanguage()
    {
        return lang.get();
    }

private:
    std::unique_ptr<Language> lang;
    cpp17::pmr::deque<Node> nodes;
    Node *root = nullptr;
};

void print(const Node &node);

std::vector<Node *> postOrder(Node &root);

void reduceTreesCoarse(Node *T1, Node *T2);

std::string printSubTree(const Node &root, bool withComments);

bool canBeFlattened(const Node *parent, const Node *child, int level);

bool isUnmovable(const Node *x);

bool hasMoveableItems(const Node *x);

bool isContainer(const Node *x);

bool canForceLeafMatch(const Node *x, const Node *y);

void markTreeAsMoved(Node *node);

#endif // ZOGRASCOPE__TREE_HPP__
