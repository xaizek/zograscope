#include "tests.hpp"

#include "Catch/catch.hpp"

#include <functional>
#include <string>

#include "parser.hpp"
#include "tree.hpp"

Tree
makeTree(const std::string &str, bool stree)
{
    TreeBuilder tb = parse(str);
    REQUIRE_FALSE(tb.hasFailed());
    return stree
         ? Tree(str, tb.makeSTree(str))
         : Tree(str, tb.getRoot());
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
