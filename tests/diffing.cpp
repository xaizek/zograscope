#include "Catch/catch.hpp"

#include <functional>

#include "TreeBuilder.hpp"
#include "parser.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"

static Node makeTree(const std::string &str);
static const Node * findNode(const Node &root, Type type);

TEST_CASE("Comment is marked as unmodified", "[comparison][postponed]")
{
    Node oldTree = makeTree(R"(
        void func() {
            /* Comment. */
            something();
            if (0) { }
            func();
        }
    )");
    Node newTree = makeTree(R"(
        void func() {
            /* Comment. */
            if(0) { }
            func();
        }
    )");

    ted(oldTree, newTree);

    CHECK(findNode(oldTree, Type::Comments)->state == State::Unchanged);
    CHECK(findNode(newTree, Type::Comments)->state == State::Unchanged);
}

static Node
makeTree(const std::string &str)
{
    TreeBuilder tb = parse(str);
    REQUIRE_FALSE(tb.hasFailed());
    return materializeTree(str, tb.getRoot());
}

static const Node *
findNode(const Node &root, Type type)
{
    const Node *needle = nullptr;

    std::function<bool(const Node &)> visit = [&](const Node &node) {
        if (node.type == type) {
            needle = &node;
            return true;
        }

        for (const Node &child : node.children) {
            if (visit(child)) {
                return true;
            }
        }

        return false;
    };

    visit(root);
    return needle;
}
