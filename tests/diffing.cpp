#include "Catch/catch.hpp"

#include <functional>

#include "TreeBuilder.hpp"
#include "change-distilling.hpp"
#include "compare.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

#include "tests.hpp"

enum class Changes
{
    No,
    Additions,
    Deletions,
    Mixed,
};

static int countLeaves(const Node &root, State state);
static int countSatellites(const Node &root);
static std::vector<Changes> makeChangeMap(Node &root);

TEST_CASE("Comment is marked as unmodified", "[comparison][postponed]")
{
    Tree oldTree = makeTree(R"(
        void func() {
            /* Comment. */
            something();
            if (0) { }
            func();
        }
    )");
    Tree newTree = makeTree(R"(
        void func() {
            /* Comment. */
            if(0) { }
            func();
        }
    )");

    ted(*oldTree.getRoot(), *newTree.getRoot());

    CHECK(findNode(oldTree, Type::Comments)->state == State::Unchanged);
    CHECK(findNode(newTree, Type::Comments)->state == State::Unchanged);
}

TEST_CASE("Removal of accessor is identified as such", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        void func()
        {
            menu_state.d->state = NULL;
        }
    )");
    Tree newTree = makeTree(R"(
        void func()
        {
            menu_state.d = NULL;
        }
    )");

    ted(*oldTree.getRoot(), *newTree.getRoot());

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
    Tree oldTree = makeTree(R"(
        // comment1
        void func() { abc; }

        // comment3
        void f() { }
    )");
    Tree newTree = makeTree(R"(
        // comment1
        void func() { xyz; }

        // comment4
        void f() { }
    )");

    ted(*oldTree.getRoot(), *newTree.getRoot());

    CHECK(findNode(oldTree, Type::UserTypes, "abc")->state == State::Updated);
    CHECK(findNode(newTree, Type::UserTypes, "xyz")->state == State::Updated);
    CHECK(findNode(oldTree, Type::Comments, "// comment3")->state
          == State::Updated);
    CHECK(findNode(newTree, Type::Comments, "// comment4")->state
          == State::Updated);
}

TEST_CASE("Fine reduction doesn't crash", "[comparison][reduction][crash]")
{
    Tree oldTree = makeTree(R"(
        void func()
        {
            /* Comment. */
            func();
            func();
        }
    )");
    Tree newTree = makeTree(R"(
        void func()
        {
            func();
        }
    )");

    Node *oldT = oldTree.getRoot(), *newT = newTree.getRoot();
    reduceTreesFine(oldT, newT);
}

TEST_CASE("Fine reduction works", "[comparison][reduction]")
{
    Tree oldTree = makeTree(R"(
        int a;

        const cmd_add_t cmds_list[] = {
            { .name = "",                  .abbr = NULL,    .id = COM_GOTO,
              .descr = "navigate to specific line",
              .flags = HAS_RANGE | HAS_COMMENT,
              .handler = &goto_cmd,        .min_args = 0,   .max_args = 0, },
        };
    )");
    Tree newTree = makeTree(R"(
        int a;

        const cmd_add_t cmds_list[] = {
            { .name = "",                  .abbr = NULL,    .id = COM_GOTO,
              .descr = "navigate to specific line",
              .flags = HAS_RANGE,
              .handler = &goto_cmd,        .min_args = 0,   .max_args = 0, },
        };
    )");

    Node *oldT = oldTree.getRoot(), *newT = newTree.getRoot();
    reduceTreesFine(oldT, newT);
    CHECK(oldT != oldTree.getRoot());
    CHECK(newT != newTree.getRoot());
    CHECK(countSatellites(*oldTree.getRoot()) == 25);
    CHECK(countSatellites(*newTree.getRoot()) == 25);
}

TEST_CASE("Reduced tree is compared correctly", "[comparison][reduction]")
{
    Tree oldTree = makeTree(R"(
        void f()
        {
            if (variable < 2 || condition)
                return;
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f()
        {
            if (condition)
                return;
        }
    )", true);

    Node *oldT = oldTree.getRoot(), *newT = newTree.getRoot();
    reduceTreesFine(oldT, newT);
    distill(*oldT, *newT);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 1);
}

TEST_CASE("Different trees are recognized as different", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        aggregate var = {
            { .field = 1 },
        };
    )", true);
    Tree newTree = makeTree(R"(
        aggregate var = {
            { .field = 2 },
        };
    )", true);

    Node *oldT = oldTree.getRoot(), *newT = newTree.getRoot();
    reduceTreesFine(oldT, newT);
    distill(*oldT, *newT);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 0);
}

TEST_CASE("Spaces are ignored during comparsion", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        void f() {
            while (condition1) {
                if (condition2) {
                    (void)ioe_errlst_append(&args->result.errors, dst, errno,
                            "Write to destination file failed");
                    error = 1;
                    break;
                }
            }
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f() {
                while (condition1) {
                    if (condition2) {
                        (void)ioe_errlst_append(&args->result.errors, dst,
                                errno, "Write to destination file failed");
                        error = 1;
                        break;
                    }
                }

                if (fflush(out) != 0) {
                    (void)ioe_errlst_append(&args->result.errors, dst, errno,
                            "Write to destination file failed");
                    error = 1;
                }
        }
    )", true);

    Node *oldT = oldTree.getRoot(), *newT = newTree.getRoot();
    distill(*oldT, *newT);

    std::vector<Changes> oldMap = makeChangeMap(*oldT);
    std::vector<Changes> newMap = makeChangeMap(*newT);

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No, Changes::No, Changes::No, Changes::No, Changes::No,
        Changes::No, Changes::No, Changes::No, Changes::No, Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No, Changes::No, Changes::No, Changes::No, Changes::No,
        Changes::No, Changes::No, Changes::No, Changes::No, Changes::No,

        Changes::Mixed,
        Changes::Additions, Changes::Additions, Changes::Additions,
        Changes::No,

        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Only similar enough functions are matched", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        int f() {
            int i = 3;
            return i;
        }
    )", true);
    Tree newTree = makeTree(R"(
        int f() {
            return f_internal();
        }

        static int f_internal() {
            int i = 3;
            return i;
        }
    )", true);

    TimeReport tr;
    Node *oldT = oldTree.getRoot(), *newT = newTree.getRoot();
    compare(oldT, newT, tr, true, false);

    std::vector<Changes> oldMap = makeChangeMap(*oldT);
    std::vector<Changes> newMap = makeChangeMap(*newT);

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No, Changes::No, Changes::No, Changes::No
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::No,
        Changes::No,
        Changes::Mixed,
        Changes::No,
        Changes::No,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Results of coarse comparison are refined with fine", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        void f(int a,
               int b)
        {
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f(int a,
               int c)
        {
        }
    )", true);

    TimeReport tr;
    Node *oldT = oldTree.getRoot(), *newT = newTree.getRoot();
    compare(oldT, newT, tr, true, false);

    std::vector<Changes> oldMap = makeChangeMap(*oldT);
    std::vector<Changes> newMap = makeChangeMap(*newT);

    std::vector<Changes> expected = { Changes::No,
        Changes::No, Changes::Mixed, Changes::No, Changes::No
    };

    CHECK(oldMap == expected);
    CHECK(newMap == expected);
}

TEST_CASE("Initializer is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        aggregate var = {
            { .field = 1 },
            { .another_field = 1 },
        };
    )", true);
    Tree newTree = makeTree(R"(
        aggregate var = {
            { .field = 2 },
            { .another_field = 2 },
        };
    )", true);

    distill(*oldTree.getRoot(), *newTree.getRoot());

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 2);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 2);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 0);
}

TEST_CASE("Structure is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        struct s {
            int a;
            int b : 1;
        };
    )", true);
    Tree newTree = makeTree(R"(
        struct s {
            int g;
            int b : 2;
        };
    )", true);

    distill(*oldTree.getRoot(), *newTree.getRoot());

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 1);
}

TEST_CASE("Enumeration is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        enum {
            Aa,
            Bb,
        };
    )", true);
    Tree newTree = makeTree(R"(
        enum {
            Ab,
            Zz
        };
    )", true);

    distill(*oldTree.getRoot(), *newTree.getRoot());

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 2);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 2);
}

TEST_CASE("Node type is propagated", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        void func()
        {
            /* Comment. */
            return;
        }
    )", true);
    Tree newTree = makeTree(R"(
        void func()
        {
            char array[1];
            /* Comment. */
            return;
        }
    )", true);

    distill(*oldTree.getRoot(), *newTree.getRoot());

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 2);
}

TEST_CASE("Coarse nodes are formed correctly", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        int var;
    )", true);
    Tree newTree = makeTree(R"(
        // Comment.
        int var;
    )", true);

    distill(*oldTree.getRoot(), *newTree.getRoot());

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 1);
}

TEST_CASE("Compound statement doesn't unite statements",
          "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        void f() {
            if(condition1) {
                if(condition2) {
                }
            }
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f() {
            if(condition1) {
                // comment
                if(condition2) {
                }
            }
        }
    )", true);

    distill(*oldTree.getRoot(), *newTree.getRoot());

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 1);
}

static int
countLeaves(const Node &root, State state)
{
    int count = 0;

    std::function<void(const Node &)> visit = [&](const Node &node) {
        if (node.children.empty() && node.state == state) {
            ++count;
        }

        for (const Node *child : node.children) {
            visit(*child);
        }
    };

    visit(root);
    return count;
}

static int
countSatellites(const Node &root)
{
    int count = 0;

    std::function<void(const Node &)> visit = [&](const Node &node) {
        if (node.satellite) {
            ++count;
        }

        for (const Node *child : node.children) {
            visit(*child);
        }
    };

    visit(root);
    return count;
}

static std::vector<Changes>
makeChangeMap(Node &root)
{
    std::vector<Changes> map;

    auto updateMap = [&](int line, const Node &node) {
        if (map.size() <= static_cast<unsigned int>(line)) {
            map.resize(line + 1);
        }

        Changes change = Changes::No;
        switch (node.state) {
            case State::Unchanged: change = Changes::No; break;
            case State::Deleted:   change = Changes::Deletions; break;
            case State::Inserted:  change = Changes::Additions; break;
            case State::Updated:   change = Changes::Mixed; break;
        }

        if (map[line] == Changes::No) {
            map[line] = change;
        } else if (map[line] != change) {
            map[line] = Changes::Mixed;
        }
    };

    std::function<void(Node &, State)> mark = [&](Node &node, State state) {
        node.state = state;
        for (Node *child : node.children) {
            mark(*child, state);
        }
    };

    int line;
    std::function<void(Node &)> visit = [&](Node &node) {
        if (node.line != 0 && node.col != 0) {
            line = node.line - 1;

            if (node.next != nullptr) {
                if (node.state != State::Unchanged) {
                    mark(*node.next, node.state);
                }
                return visit(*node.next);
            }

            const std::vector<std::string> lines = split(node.label, '\n');
            updateMap(line, node);
            for (std::size_t i = 1U; i < lines.size(); ++i) {
                updateMap(++line, node);
            }
        }

        for (Node *child : node.children) {
            visit(*child);
        }
    };
    visit(root);

    return map;
}
