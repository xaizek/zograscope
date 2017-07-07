#include "TreeBuilder.hpp"

#include <cstddef>

#include <deque>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

PNode *
TreeBuilder::addNode(Text value, const Location &loc, int token, SType stype)
{
    value.token = token;

    if (value.postponedFrom != value.postponedTo) {
        std::vector<PNode *> children;
        children.reserve(value.postponedTo - value.postponedFrom);
        for (std::size_t i = value.postponedFrom; i < value.postponedTo; ++i) {
            nodes.emplace_back(postponed[i].value, postponed[i].loc,
                               SType::Postponed, true);
            children.push_back(&nodes.back());
        }
        nodes.emplace_back(value, loc, stype);
        children.push_back(&nodes.back());
        nodes.emplace_back(std::move(children));
        return &nodes.back();
    }

    nodes.emplace_back(value, loc, stype);
    return &nodes.back();
}

PNode *
TreeBuilder::addNode(std::vector<PNode *> children, SType stype)
{
    movePostponed(children[0], children, children.cbegin());
    nodes.emplace_back(std::move(children), stype);
    return &nodes.back();
}

void
TreeBuilder::finish(bool failed)
{
    if (failed) {
        this->failed = failed;
        return;
    }

    std::vector<PNode *> children;
    children.reserve(newPostponed);
    for (std::size_t i = postponed.size() - newPostponed; i < postponed.size();
         ++i) {
        nodes.emplace_back(postponed[i].value, postponed[i].loc,
                           SType::Postponed, true);
        children.push_back(&nodes.back());
    }

    root->children.insert(root->children.cend(),
                          children.cbegin(), children.cend());
}

static void
print(const PNode *node, const std::string &contents, std::vector<bool> &state)
{
    std::cout << (state.empty() ? "--- " : "    ");

    for (unsigned int i = 0U; i < state.size(); ++i) {
        const bool last = (i == state.size() - 1U);
        if (state[i]) {
            std::cout << (last ? "`-- " : "    ");
        } else {
            std::cout << (last ? "|-- " : "|   ");
        }
    }

    // std::cout << '(' << node->line << ';' << node->col << ")\n";
    std::cout << contents.substr(node->value.from, node->value.len)
              << ' ' << (int)node->stype
              << '\n';

    state.push_back(false);
    for (unsigned int i = 0U; i < node->children.size(); ++i) {
        state.back() = (i == node->children.size() - 1U);
        print(node->children[i], contents, state);
    }
    state.pop_back();
}

static void
print(const PNode *node, const std::string &contents)
{
    std::vector<bool> state;
    print(node, contents, state);
}

static PNode *
findSNode(PNode *node)
{
    if (node->stype != SType::None) {
        return node;
    }

    return node->children.size() == 1U
         ? findSNode(node->children.front())
         : nullptr;
}

SNode *
TreeBuilder::makeSTree(const std::string &contents, bool dumpWhole,
                       bool dumpUnclear)
{
    if (dumpWhole) {
        print(root, contents);
    }

    PNode *rootNode = findSNode(root);
    if (rootNode == nullptr) {
        snodes.emplace_back(SNode{root, {}});
        return &snodes[0];
    }

    std::function<SNode *(PNode *)> makeSNode = [&, this](PNode *node) {
        auto isSNode = [](PNode *child) {
            return (findSNode(child) != nullptr);
        };
        // If none of the children is SNode, then current node is a leaf SNode.
        if (std::none_of(node->children.begin(), node->children.end(),
                         isSNode)) {
            snodes.emplace_back(SNode{node, {}});
            return &snodes.back();
        }

        std::vector<SNode *> c;
        c.reserve(node->children.size());
        for (PNode *child : node->children) {
            if (PNode *schild = findSNode(child)) {
                c.push_back(makeSNode(schild));
            } else {
                if (dumpUnclear) {
                    print(child, contents);
                }
                snodes.emplace_back(SNode{node->children[c.size()], {}});
                c.push_back(&snodes.back());
            }
        }
        snodes.emplace_back(SNode{node, std::move(c)});
        return &snodes.back();
    };

    return makeSNode(rootNode);
}

void
TreeBuilder::movePostponed(PNode *&node, std::vector<PNode *> &nodes,
                           std::vector<PNode *>::const_iterator insertPos)
{
    auto pos = std::partition_point(node->children.begin(),
                                    node->children.end(),
                                    [](PNode *n) { return n->postponed; });
    if (pos == node->children.begin()) {
        return;
    }

    std::vector<PNode *> postponed(node->children.begin(), pos);
    node->children.erase(node->children.begin(), pos);

    if (node->children.front()->empty() && node->children.size() == 1U) {
        node->children.front()->stype = node->stype;
        node = node->children.front();
    }

    nodes.insert(insertPos, postponed.cbegin(), postponed.cend());
}
