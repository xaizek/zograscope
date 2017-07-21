#include "tests.hpp"

#include "Catch/catch.hpp"

#include <functional>
#include <string>
#include <utility>

#include "STree.hpp"
#include "parser.hpp"
#include "tree.hpp"

Tree
makeTree(const std::string &str, bool coarse)
{
    TreeBuilder tb = parse(str);
    REQUIRE_FALSE(tb.hasFailed());

    if (!coarse) {
        return Tree(str, tb.getRoot());
    }

    STree stree(std::move(tb), str);
    return Tree(str, stree.getRoot());
}

const Node *
findNode(const Tree &tree, Type type, const std::string &label)
{
    const Node *needle = nullptr;

    std::function<bool(const Node *)> visit = [&](const Node *node) {
        if (node->type == type) {
            if (label.empty() || node->label == label) {
                needle = node;
                return true;
            }
        }

        for (const Node *child : node->children) {
            if (visit(child)) {
                return true;
            }
        }

        return false;
    };

    visit(tree.getRoot());
    return needle;
}
