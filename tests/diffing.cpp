#include "Catch/catch.hpp"

#include <functional>

#include "TreeBuilder.hpp"
#include "change-distilling.hpp"
#include "parser.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"

static Node makeTree(const std::string &str, bool stree = false);
static const Node * findNode(const Node &root, Type type,
                             const std::string &label = {});
static int countLeaves(const Node &root, State state);

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

TEST_CASE("Reduction doesn't crash", "[comparison][reduction][crash]")
{
    Node oldTree = makeTree(R"(
        void func()
        {
            /* Comment. */
            func();
            func();
        }
    )");
    Node newTree = makeTree(R"(
        void func()
        {
            func();
        }
    )");

    ted(oldTree, newTree);
}

TEST_CASE("Reduced tree is compared correctly", "[comparison][reduction]")
{
    Node oldTree = makeTree(R"(
        void f()
        {
            if (variable < 2 || condition)
                return;
        }
    )", true);
    Node newTree = makeTree(R"(
        void f()
        {
            if (condition)
                return;
        }
    )", true);

    Node *oldT = &oldTree, *newT = &newTree;
    reduceTreesFine(oldT, newT);
    distill(*oldT, *newT);

    CHECK(countLeaves(oldTree, State::Updated) == 0);
    CHECK(countLeaves(oldTree, State::Deleted) == 1);
    CHECK(countLeaves(oldTree, State::Inserted) == 0);

    CHECK(countLeaves(newTree, State::Updated) == 0);
    CHECK(countLeaves(newTree, State::Deleted) == 0);
    CHECK(countLeaves(newTree, State::Inserted) == 1);
}

TEST_CASE("Node type is propagated", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Node oldTree = makeTree(R"(
        void func()
        {
            /* Comment. */
            return;
        }
    )", true);
    Node newTree = makeTree(R"(
        void func()
        {
            char array[1];
            /* Comment. */
            return;
        }
    )", true);

    distill(oldTree, newTree);

    CHECK(countLeaves(oldTree, State::Updated) == 0);
    CHECK(countLeaves(oldTree, State::Deleted) == 0);
    CHECK(countLeaves(oldTree, State::Inserted) == 0);

    CHECK(countLeaves(newTree, State::Updated) == 0);
    CHECK(countLeaves(newTree, State::Deleted) == 0);
    CHECK(countLeaves(newTree, State::Inserted) == 1);
}

TEST_CASE("Coarse nodes are formed correctly", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Node oldTree = makeTree(R"(
        int var;
    )", true);
    Node newTree = makeTree(R"(
        // Comment.
        int var;
    )", true);

    distill(oldTree, newTree);

    CHECK(countLeaves(oldTree, State::Updated) == 0);
    CHECK(countLeaves(oldTree, State::Deleted) == 0);
    CHECK(countLeaves(oldTree, State::Inserted) == 0);

    CHECK(countLeaves(newTree, State::Updated) == 0);
    CHECK(countLeaves(newTree, State::Deleted) == 0);
    CHECK(countLeaves(newTree, State::Inserted) == 1);
}

static Node
makeTree(const std::string &str, bool stree)
{
    TreeBuilder tb = parse(str);
    REQUIRE_FALSE(tb.hasFailed());
    return stree
         ? materializeTree(str, tb.makeSTree())
         : materializeTree(str, tb.getRoot());
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

static int
countLeaves(const Node &root, State state)
{
    int count = 0;

    std::function<void(const Node &)> visit = [&](const Node &node) {
        if (node.children.empty() && node.state == state) {
            ++count;
        }

        for (const Node &child : node.children) {
            visit(child);
        }

        return false;
    };

    visit(root);
    return count;
}
