#include "Catch/catch.hpp"

#include <functional>
#include <ostream>

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
    Moves,
    Mixed,
};

static int countLeaves(const Node &root, State state);
static std::vector<Changes> makeChangeMap(Node &root);
static std::ostream & operator<<(std::ostream &os, Changes changes);

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

TEST_CASE("All postponed nodes are pulled into parents",
          "[comparison][postponed]")
{
    Tree oldTree = makeTree(R"(
        typedef struct
        {
            int a; // Comment 1.
        }
        str;
    )");
    Tree newTree = makeTree(R"(
        typedef struct
        {
            int a; // Comment 1.
            int b; // Comment 2.
        }
        str;
    )");

    ted(*oldTree.getRoot(), *newTree.getRoot());

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) > 0);

    CHECK(findNode(newTree, Type::Comments, "// Comment 2.")->state
          == State::Inserted);
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

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

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

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 1);
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

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No, Changes::No, Changes::No, Changes::No, Changes::No,
        Changes::No, Changes::No, Changes::No, Changes::No, Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No, Changes::No, Changes::No, Changes::No, Changes::No,
        Changes::No, Changes::No, Changes::No, Changes::No, Changes::No,

        Changes::Additions,
        Changes::Additions, Changes::Additions, Changes::Additions,
        Changes::Additions,

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
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, false);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::Moves,
        Changes::Moves,
        Changes::No
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::Moves,
        Changes::Moves,
        Changes::Additions,
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

TEST_CASE("Functions are matched using best match algorithm", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        void f()
        {
            column_data_t cdt = {
                .entry = that,
                .line_pos = old_pos,
                .is_current = is_current,
            };

            cdt.column_offset = 1;
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f()
        {
            const column_data_t cdt = {
                .entry = this,
                .line_pos = i,
                .is_current = 0,
            };
        }

        void f()
        {
            column_data_t cdt = {
                .entry = that,
                .line_pos = old_pos,
                .is_current = is_current,
            };

            cdt.column_offset = 2;
        }
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, false);

    // If functions matched correctly, only one leaf of the old tree will be
    // removed.
    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);
}

TEST_CASE("Function specifiers are detected as such", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree("TYPEMACRO void f();", true);
    Tree newTree = makeTree("void f();", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 0);
}

TEST_CASE("Flat initializer is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        type var = {
            .oldfield = oldValue,
        };
    )", true);
    Tree newTree = makeTree(R"(
        type var = {
            .oldfield = oldValue,
            .newField = newValue
        };
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Nested initializer is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        aggregate var = {
            { .field =
              1
            }, { .another_field =
              1
            },
        };
    )", true);
    Tree newTree = makeTree(R"(
        aggregate var = {
            { .field =
              2
            }, { .another_field =
              2
            },
        };
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::Deletions,
        Changes::No,
        Changes::Deletions,
        Changes::No,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::No,
        Changes::Additions,
        Changes::No,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Structure is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        struct s {
            int a;
            int b :
            1
            ;
        };
    )", true);
    Tree newTree = makeTree(R"(
        struct s {
            int g;
            int b :
            2
            ;
        };
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, false);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::Deletions,
        Changes::No,
        Changes::Deletions,
        Changes::No,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::No,
        Changes::Additions,
        Changes::No,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Structure with one element is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        typedef struct
        {
            int a;
        }
        str;
    )", true);
    Tree newTree = makeTree(R"(
        // Comment
        typedef struct
        {
            int a;
            int b;
        }
        str;
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, false);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::Additions,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::No,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Enumeration is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        enum {
            A,
            B,
            Aa,
            Bb,
        };
    )", true);
    Tree newTree = makeTree(R"(
        enum {
            A,
            B,
            Ab,
            Zz
        };
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::Mixed,
        Changes::Mixed,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::Mixed,
        Changes::Additions,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
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

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 1);
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

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 1);
}

TEST_CASE("Declarations differ by whether they have initializers",
          "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    Tree oldTree = makeTree(R"(
        void f() {
            int a = {};
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f() {
            int a;
        }
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) > 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) > 0);
}

TEST_CASE("Declarations with and without initializer are not the same",
          "[comparison]")
{
    Tree oldTree = makeTree(R"(
        int
            a
            ;
    )", true);
    Tree newTree = makeTree(R"(
        int
            a
            =
            10
            ;
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::Deletions,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::Additions,
        Changes::Additions,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Move detection isn't thrown off by large changes",
          "[comparison][moves]")
{
    Tree oldTree = makeTree(R"(
        void f()
        {
            stmt1;
            stmt2;
            stmt3;
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f()
        {
            stmt1;
            {
                call();
                call();
                call();
                call();
                call();
                call();
                call();
                stmt2;
            }
            stmt3;
        }
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::Moves,
        Changes::No,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::Additions,
        Changes::Additions,
        Changes::Additions,
        Changes::Additions,
        Changes::Additions,
        Changes::Additions,
        Changes::Additions,
        Changes::Moves,
        Changes::Additions,
        Changes::No,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Move detection works on top level", "[comparison][moves]")
{
    Tree oldTree = makeTree(R"(
        #include "this.h"
        #include "bla.h"

        #include "some.h"
        #include "file.h"
        #include "here.h"
    )", true);
    Tree newTree = makeTree(R"(
        #include "bla.h"
        #include "this.h"

        #include "here.h"
        #include "file.h"
        #include "some.h"
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::Moves,
        Changes::No,
        Changes::No,
        Changes::Moves,
        Changes::Moves,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::Moves,
        Changes::No,
        Changes::No,
        Changes::Moves,
        Changes::Moves,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Move detection works across nested nodes", "[comparison][moves]")
{
    Tree oldTree = makeTree(R"(
        void f() {
            if (cond) {
                a = b;
                b = c;
            }
        }

        void g() {
            if (cond) {
                int a;
                int b;
            }
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f() {
            a = b;
            if (cond) {
                b = c;
            }
        }

        void g() {
            int a;
            if (cond) {
                int b;
            }
        }
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No, Changes::No,
        Changes::Moves,
        Changes::No, Changes::No, Changes::No,
        Changes::No, Changes::No, Changes::No,
        Changes::Moves,
        Changes::No, Changes::No, Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::Moves,
        Changes::No, Changes::No, Changes::No,
        Changes::No, Changes::No, Changes::No,
        Changes::Moves,
        Changes::No, Changes::No, Changes::No, Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Unchanged elements are those which compare equal", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        // Comment.
        struct s {
            gid_t gid;
        };
    )", true);
    Tree newTree = makeTree(R"(
        // Comment.
        struct s {
            id_t gid;
        };
    )", true);

    distill(*oldTree.getRoot(), *newTree.getRoot());

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 0);
}

TEST_CASE("Else branch addition", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        void f()
        {
            if(condition)
            {
                action2();
            }
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f()
        {
            if(condition)
            {
                action1();
            }
            else
            {
                action3();
            }
        }
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::Mixed,
        Changes::No,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::Mixed,
        Changes::No,
        Changes::Additions,
        Changes::Additions,
        Changes::Additions,
        Changes::Additions,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Else branch removal", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        void f()
        {
            if(condition)
            {
                action1();
            }
            else
            {
                action3();
            }
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f()
        {
            if(condition)
            {
                action2();
            }
        }
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::Mixed,
        Changes::No,
        Changes::Deletions,
        Changes::Deletions,
        Changes::Deletions,
        Changes::Deletions,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::Mixed,
        Changes::No,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Preserved child preserves its parent", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        void f() {
            if (cond)
            {
                computation();
            }
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f() {
            if (a < 88)
            {
                newstep();
                computation();
                newstep();
            }
        }
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::Deletions,
        Changes::Moves,
        Changes::Moves,
        Changes::Moves,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::Moves,
        Changes::Additions,
        Changes::Moves,
        Changes::Additions,
        Changes::Moves,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Parent nodes bind leaves on matching", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        void f() {
            nread = fscanf(f, "%30d\n", &num);
            if(nread != 1) {
                assert(fsetpos(f, &pos) == 0 && "Failed");
                return -1;
            }
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f() {
            if(c == EOF) {
                return -1;
            }

            nread = fscanf(f, "%30d\n", &num);
            if(nread != 1) {
                fsetpos(f, &pos);
                return -1;
            }
        }
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::Deletions,
        Changes::No,
        Changes::No,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::Additions,
        Changes::Additions,
        Changes::No,
        Changes::No,
        Changes::No,
        Changes::Additions,
        Changes::No,
        Changes::No,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Functions are matched by content also", "[comparison]")
{
    Tree oldTree = makeTree(R"(
        entries_t f() {
            entries_t parent_dirs = {};
            char *path;
            int len, i;
            char **list;

            list = list_all_files(path, &len);

            free(path);
            free_string_array(list, len);

            return parent_dirs;
        }
    )", true);
    Tree newTree = makeTree(R"(
        entries_t f() {
            entries_t parent_dirs = g();
            if(parent_dirs.nentries < 0) {
                return parent_dirs;
            }

            return parent_dirs;
        }

        entries_t g() {
            entries_t siblings = {};
            char *path;
            int len, i;
            char **list;

            list = list_all_files(path, &len);

            free(path);
            free_string_array(list, len);

            return siblings;
        }
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, false);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::Mixed, Changes::Deletions,
        Changes::No, Changes::No, Changes::No, Changes::No, Changes::No,
        Changes::No, Changes::No, Changes::No, Changes::No,
        Changes::Deletions,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::Additions, Changes::Additions, Changes::Additions,
        Changes::Additions, Changes::Additions,
        Changes::No,
        Changes::Additions, Changes::Additions,
        Changes::No,
        Changes::Mixed, Changes::Additions,
        Changes::No, Changes::No, Changes::No, Changes::No, Changes::No,
        Changes::No, Changes::No, Changes::No, Changes::No,
        Changes::Additions,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

TEST_CASE("Removed/added subtrees aren't marked moved", "[comparison][moves]")
{
    Tree oldTree = makeTree(R"(
        void f() {
            if (cond1) { }
            else
                if (cond3) { stmt; }
        }
    )", true);
    Tree newTree = makeTree(R"(
        void f() {
            if (cond1) { }
            if (cond3) { stmt; }
        }
    )", true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());

    std::vector<Changes> expectedOld = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::Deletions,
        Changes::Moves,
        Changes::No,
    };
    std::vector<Changes> expectedNew = { Changes::No,
        Changes::No,
        Changes::No,
        Changes::Moves,
        Changes::No,
    };

    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

static int
countLeaves(const Node &root, State state)
{
    int count = 0;

    std::function<void(const Node &)> visit = [&](const Node &node) {
        if (node.state == State::Unchanged && node.next != nullptr) {
            return visit(*node.next);
        }

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
            case State::Unchanged:
                change = (node.moved ? Changes::Moves : Changes::No); break;
            case State::Deleted:  change = Changes::Deletions; break;
            case State::Inserted: change = Changes::Additions; break;
            case State::Updated:  change = Changes::Mixed; break;
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
        if (node.next != nullptr) {
            if (node.state != State::Unchanged) {
                mark(*node.next, node.state);
            }
            if (node.moved) {
                markTreeAsMoved(node.next);
            }
            return visit(*node.next);
        }

        if (node.line != 0 && node.col != 0) {
            line = node.line - 1;
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

static std::ostream &
operator<<(std::ostream &os, Changes changes)
{
    switch (changes) {
        case Changes::No:        return (os << "No");
        case Changes::Additions: return (os << "Additions");
        case Changes::Deletions: return (os << "Deletions");
        case Changes::Moves:     return (os << "Moves");
        case Changes::Mixed:     return (os << "Mixed");
    }

    return (os << "Unknown Changes value");
}
