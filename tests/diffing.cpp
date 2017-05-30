#include "Catch/catch.hpp"

#include <functional>

#include "TreeBuilder.hpp"
#include "parser.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"

static Node makeTree(const std::string &str);
static const Node * findNode(const Node &root, Type type,
                             const std::string &label = {});

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

TEST_CASE("Removal of accessor is identified as such", "[comparison]")
{
    Node oldTree = makeTree(R"(
        void func()
        {
            menu_state.d->state = NULL;
        }
    )");
    Node newTree = makeTree(R"(
        void func()
        {
            menu_state.d = NULL;
        }
    )");

    ted(oldTree, newTree);

    CHECK(findNode(oldTree, Type::Identifiers, "menu_state")->state
          == State::Unchanged);
    CHECK(findNode(oldTree, Type::Other, ".")->state == State::Unchanged);
    CHECK(findNode(oldTree, Type::Other, "->")->state == State::Deleted);
    CHECK(findNode(oldTree, Type::Identifiers, "state")->state
          == State::Deleted);
}

TEST_CASE("Changes are detected in presence of comments",
          "[comparison][postponed]")
{
    Node oldTree = makeTree(R"(
        // comment1
        void func() { abc; }

        // comment3
        void f() { }
    )");
    Node newTree = makeTree(R"(
        // comment1
        void func() { xyz; }

        // comment4
        void f() { }
    )");

    ted(oldTree, newTree);

    CHECK(findNode(oldTree, Type::UserTypes, "abc")->state == State::Updated);
    CHECK(findNode(newTree, Type::UserTypes, "xyz")->state == State::Updated);
    CHECK(findNode(oldTree, Type::Comments, "// comment3")->state
          == State::Updated);
    CHECK(findNode(newTree, Type::Comments, "// comment4")->state
          == State::Updated);
}

static Node
makeTree(const std::string &str)
{
    TreeBuilder tb = parse(str);
    REQUIRE_FALSE(tb.hasFailed());
    return materializeTree(str, tb.getRoot());
}

static const Node *
findNode(const Node &root, Type type, const std::string &label)
{
    const Node *needle = nullptr;

    std::function<bool(const Node &)> visit = [&](const Node &node) {
        if (node.type == type) {
            if (label.empty() || node.label == label) {
                needle = &node;
                return true;
            }
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
