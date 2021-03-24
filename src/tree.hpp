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

#ifndef ZOGRASCOPE__TREE_HPP__
#define ZOGRASCOPE__TREE_HPP__

#include <boost/utility/string_ref.hpp>

#include <cstdint>

#include <memory>
#include <string>
#include <vector>

#include "pmr/pmr_deque.hpp"
#include "pmr/pmr_vector.hpp"

#include "utils/Pool.hpp"
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

    boost::string_ref label;
    boost::string_ref spelling;
    cpp17::pmr::vector<Node *> children;
    Node *relative = nullptr;
    Node *parent = nullptr;
    Node *next = nullptr;
    int valueChild = -1;
    int poID = -1; // Post-order ID.
    int line = 0;
    int col = 0;
    Type type : 8;
    SType stype : 8;
    State state : 8;
    bool satellite : 1; // Decorative element or node whose match was finalized.
    bool moved : 1;
    bool last : 1; // This is root of a tree from the last layer.
    bool leaf : 1; // This node corresponds to something in the source.

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
    Tree(allocator_type al = {}) : nodes(al), stringified(al), internPool(al)
    { }
    Tree(const Tree &rhs) = delete;
    Tree(Tree &&rhs) = default;
    Tree(std::unique_ptr<Language> lang, const std::string &contents,
         const PNode *node, allocator_type al = {});
    Tree(std::unique_ptr<Language> lang, const std::string &contents,
         const SNode *node, allocator_type al = {});

    Tree & operator=(const Tree &rhs) = delete;
    Tree & operator=(Tree &&rhs) = default;

public:
    // Checks whether the tree is empty and thus shouldn't be used.
    bool isEmpty() const
    {
        return (root == nullptr);
    }

    Node * getRoot()
    {
        return root;
    }

    const Node * getRoot() const
    {
        return root;
    }

    // Retrieves language associated with this tree.
    Language * getLanguage()
    {
        return lang.get();
    }

    // Retrieves language associated with this tree as constant object.
    const Language * getLanguage() const
    {
        return lang.get();
    }

    // Marks nodes of the subtree as moved if that makes sense for them.
    void markTreeAsMoved(Node *node);

    // Marks all nodes with their positions on pre-order traversal.
    void markInPreOrder();

    // Dumps tree on standard output for debugging purposes.
    void dump() const;

    // Propagates states (both added/deleted and moved flags) across layers of
    // the tree.
    void propagateStates();

private:
    // Turns SNode-subtree into a corresponding Node-subtree.
    Node * materializeSNode(const std::string &contents,
                            const SNode *node, const SNode *parent);
    // Turns PNode-subtree into a corresponding Node-subtree.
    Node * materializePNode(const std::string &contents, const PNode *node);

    // Interns a string.
    boost::string_ref intern(std::string &&str);

private:
    std::unique_ptr<Language> lang;
    // Storage of all nodes managed by this unit.
    Pool<Node> nodes;
    Node *root = nullptr;
    // Storage of most labels and spelling.
    cpp17::pmr::vector<char> stringified;
    // Storage for interned strings.
    cpp17::pmr::deque<std::string> internPool;
};

std::vector<Node *> postOrder(Node &root);

void reduceTreesCoarse(Node *T1, Node *T2);

// Turns tree defined by the node into a string.
std::string printSubTree(const Node &root, bool withComments,
                         int size_hint = -1);

bool canForceLeafMatch(const Node *x, const Node *y);

// Dumps tree defined by the node into specified stream for debugging purposes.
void dumpTree(std::ostream &os, const Node *node, const Language *lang);

#endif // ZOGRASCOPE__TREE_HPP__
