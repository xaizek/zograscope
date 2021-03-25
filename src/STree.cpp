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

#include "STree.hpp"

#include <deque>
#include <iostream>
#include <utility>
#include <vector>

#include "utils/Pool.hpp"
#include "utils/trees.hpp"
#include "Language.hpp"
#include "decoration.hpp"

static void print(const PNode *node, const std::string &contents,
                  Language &lang);
static PNode * findSNode(PNode *node);
static SNode * makeSNode(Pool<SNode> &snodes, const std::string &contents,
                         Language &lang, PNode *pnode, bool dumpUnclear);

STree::STree(TreeBuilder &&ptree, const std::string &contents, bool dumpWhole,
             bool dumpUnclear, Language &lang, cpp17::pmr::monolithic &mr)
    : ptree(std::move(ptree)), pool(&mr)
{
    PNode *proot = ptree.getRoot();

    if (dumpWhole) {
        print(proot, contents, lang);
    }

    PNode *rootNode = findSNode(proot);
    if (rootNode == nullptr) {
        root = pool.make(proot);
        return;
    }

    root = makeSNode(pool, contents, lang, rootNode, dumpUnclear);
}

static void
print(const PNode *node, const std::string &contents, Language &lang)
{
    using namespace decor;
    using namespace decor::literals;

    Decoration labelHi = 78_fg + bold;
    Decoration stypeHi = 222_fg;
    Decoration badStypeHi = 88_bg;

    trees::print(std::cout, node, [&](std::ostream &os, const PNode *node) {
        bool badSType = (node->stype == SType{} && !node->children.empty());

        os << (labelHi << '`'
                       << contents.substr(node->value.from, node->value.len)
                       << '`')
           << ", "
           << ((badSType ? badStypeHi : stypeHi) << lang.toString(node->stype))
           << '\n';
    });
}

static PNode *
findSNode(PNode *node)
{
    if (node->stype != SType{}) {
        return node;
    }

    return node->children.size() == 1U
         ? findSNode(node->children.front())
         : nullptr;
}

static SNode *
makeSNode(Pool<SNode> &pool, const std::string &contents, Language &lang,
          PNode *pnode, bool dumpUnclear)
{
    SNode *snode = pool.make(pnode);

    auto isSNode = [](PNode *child) {
        return (findSNode(child) != nullptr);
    };
    // If none of the children is SNode, then current node is a leaf SNode.
    if (std::none_of(pnode->children.begin(), pnode->children.end(), isSNode)) {
        return snode;
    }

    cpp17::pmr::vector<SNode *> &c = snode->children;
    c.reserve(pnode->children.size());
    for (PNode *child : pnode->children) {
        if (PNode *schild = findSNode(child)) {
            c.push_back(makeSNode(pool, contents, lang, schild, dumpUnclear));
        } else {
            if (dumpUnclear) {
                print(child, contents, lang);
            }
            c.push_back(pool.make(pnode->children[c.size()]));
        }
    }
    return snode;
}
