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
    Updates,
    Moves,
    Mixed,
};

static int countLeaves(const Node &root, State state);
static void diffSources(const std::string &left, const std::string &right,
                        bool skipRefine);
static std::pair<std::string, std::vector<Changes>>
extractExpectations(const std::string &src);
static std::pair<std::string, std::string> splitAt(const std::string &s,
                                                   const std::string &delim);
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
    diffSources(R"(
        void f() {
            while (condition1) {
                if (condition2) {
                    (void)ioe_errlst_append(&args->result.errors, dst,
                           errno, "Write to destination file failed");
                    error = 1;
                    break;
                }
            }
        }
    )", R"(
        void f() {
            while (condition1) {
                if (condition2) {
                    (void)ioe_errlst_append(&args->result.errors, dst,
                            errno, "Write to destination file failed");
                    error = 1;
                    break;
                }
            }

            if (fflush(out) != 0) {                                /// Additions
                (void)ioe_errlst_append(&args->result.errors, dst, /// Additions
                       errno, "Write to destination file failed"); /// Additions
                error = 1;                                         /// Additions
            }                                                      /// Additions
        }
    )", true);
}

TEST_CASE("Only similar enough functions are matched", "[comparison]")
{
    diffSources(R"(
        int f() {
            int i = 3;             /// Moves
            return i;              /// Moves
        }
    )", R"(
        int f() {
            return f_internal();   /// Additions
        }

        static int f_internal() {  /// Additions
            int i = 3;             /// Moves
            return i;              /// Moves
        }                          /// Additions
    )", false);
}

TEST_CASE("Results of coarse comparison are refined with fine", "[comparison]")
{
    diffSources(R"(
        void f(int a,
               int b)  /// Mixed
        {
        }
    )", R"(
        void f(int a,
               int c)  /// Mixed
        {
        }
    )", false);
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

    diffSources(R"(
        type var = {
            .oldfield = oldValue,
        };
    )", R"(
        type var = {
            .oldfield = oldValue,
            .newField = newValue   /// Additions
        };
    )", true);
}

TEST_CASE("Nested initializer is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    diffSources(R"(
        aggregate var = {
            { .field =
              1                    /// Deletions
            }, { .another_field =
              1                    /// Deletions
            },
        };
    )", R"(
        aggregate var = {
            { .field =
              2                    /// Additions
            }, { .another_field =
              2                    /// Additions
            },
        };
    )", true);
}

TEST_CASE("Structure is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    diffSources(R"(
        struct s {
            int a;   /// Deletions
            int b :
            1        /// Deletions
            ;
        };
    )", R"(
        struct s {
            int g;   /// Additions
            int b :
            2        /// Additions
            ;
        };
    )", false);
}

TEST_CASE("Structure with one element is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    diffSources(R"(
        typedef struct
        {
            int a;
        }
        str;
    )", R"(
        // Comment      /// Additions
        typedef struct
        {
            int a;
            int b;      /// Additions
        }
        str;
    )", false);
}

TEST_CASE("Enumeration is decomposed", "[comparison][parsing]")
{
    // This is more of a parsing test, but it's easier and more reliable to test
    // it by comparison.

    diffSources(R"(
        enum {
            A,
            B,
            Aa,  /// Mixed
            Bb,  /// Mixed
        };
    )", R"(
        enum {
            A,
            B,
            Ab,  /// Mixed
            Zz   /// Updates
        };
    )", true);
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
    diffSources(R"(
        int
            a
            ;
    )", R"(
        int
            a
            =   /// Additions
            10  /// Additions
            ;
    )", true);
}

TEST_CASE("Move detection isn't thrown off by large changes",
          "[comparison][moves]")
{
    diffSources(R"(
        void f()
        {
            stmt1;
            stmt2;       /// Moves
            stmt3;
        }
    )", R"(
        void f()
        {
            stmt1;
            {            /// Additions
                call();  /// Additions
                call();  /// Additions
                call();  /// Additions
                call();  /// Additions
                call();  /// Additions
                call();  /// Additions
                call();  /// Additions
                stmt2;   /// Moves
            }            /// Additions
            stmt3;
        }
    )", true);
}

TEST_CASE("Move detection works on top level", "[comparison][moves]")
{
    diffSources(R"(
        #include "this.h"  /// Moves
        #include "bla.h"

        #include "some.h"  /// Moves
        #include "file.h"  /// Moves
        #include "here.h"
    )", R"(
        #include "bla.h"
        #include "this.h"  /// Moves

        #include "here.h"
        #include "file.h"  /// Moves
        #include "some.h"  /// Moves
    )", true);
}

TEST_CASE("Move detection works across nested nodes", "[comparison][moves]")
{
    diffSources(R"(
        void f() {
            if (cond) {
                a = b;   /// Moves
                b = c;
            }
        }

        void g() {
            if (cond) {
                int a;   /// Moves
                int b;
            }
        }
    )", R"(
        void f() {
            a = b;       /// Moves
            if (cond) {
                b = c;
            }
        }

        void g() {
            int a;       /// Moves
            if (cond) {
                int b;
            }
        }
    )", true);
}

TEST_CASE("Move detection doesn't falsely mark root children moved",
          "[comparison][moves]")
{
    diffSources(R"(
        #include <stdio.h>
    )", R"(
        #include <stdio.h>
        long_string_of_some_words_to_throw_off_comparison var;  /// Additions
    )", true);
}

TEST_CASE("Unmoved statement is detected as such", "[comparison][moves]")
{
    diffSources(R"(
        void f() {
            int i;
            int j;        /// Moves
        }
    )", R"(
        void f() {
            int i;

            if (!flag) {  /// Additions
                int j;    /// Moves
            }             /// Additions
        }
    )", true);
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
    diffSources(R"(
        void f()
        {
            if(condition)
            {
                action2();  /// Mixed
            }
        }
    )", R"(
        void f()
        {
            if(condition)
            {
                action1();  /// Mixed
            }
            else            /// Additions
            {               /// Additions
                action3();  /// Additions
            }               /// Additions
        }
    )", true);
}

TEST_CASE("Else branch removal", "[comparison]")
{
    diffSources(R"(
        void f()
        {
            if(condition)
            {
                action1();  /// Mixed
            }
            else            /// Deletions
            {               /// Deletions
                action3();  /// Deletions
            }               /// Deletions
        }
    )", R"(
        void f()
        {
            if(condition)
            {
                action2();  /// Mixed
            }
        }
    )", true);
}

TEST_CASE("Preserved child preserves its parent", "[comparison]")
{
    diffSources(R"(
        void f() {
            if (cond)           /// Deletions
            {                   /// Deletions
                computation();  /// Moves
            }                   /// Deletions
        }
    )", R"(
        void f() {
            if (a < 88)         /// Additions
            {                   /// Additions
                newstep();      /// Additions
                computation();  /// Moves
                newstep();      /// Additions
            }                   /// Additions
        }
    )", false);
}

TEST_CASE("Parent nodes bind leaves on matching", "[comparison]")
{
    diffSources(R"(
        void f() {
            nread = fscanf(f, "%30d\n", &num);
            if(nread != 1) {
                assert(fsetpos(f, &pos) == 0 && "Failed");  /// Deletions
                return -1;
            }
        }
    )", R"(
        void f() {
            if(c == EOF) {                                  /// Additions
                return -1;                                  /// Additions
            }                                               /// Additions

            nread = fscanf(f, "%30d\n", &num);
            if(nread != 1) {
                fsetpos(f, &pos);                           /// Additions
                return -1;
            }
        }
    )", true);
}

TEST_CASE("Functions are matched by content also", "[comparison]")
{
    diffSources(R"(
        entries_t f() {                         /// Mixed
            entries_t parent_dirs = {};         /// Deletions
            char *path;
            int len, i;
            char **list;

            list = list_all_files(path, &len);

            free(path);
            free_string_array(list, len);

            return parent_dirs;                 /// Deletions
        }
    )", R"(
        entries_t f() {                         /// Additions
            entries_t parent_dirs = g();        /// Additions
            if(parent_dirs.nentries < 0) {      /// Additions
                return parent_dirs;             /// Additions
            }                                   /// Additions

            return parent_dirs;                 /// Additions
        }                                       /// Additions

        entries_t g() {                         /// Mixed
            entries_t siblings = {};            /// Additions
            char *path;
            int len, i;
            char **list;

            list = list_all_files(path, &len);

            free(path);
            free_string_array(list, len);

            return siblings;                    /// Additions
        }
    )", false);
}

TEST_CASE("Removed/added subtrees aren't marked moved", "[comparison][moves]")
{
    diffSources(R"(
        void f() {
            if (cond1) { }
            else                      /// Deletions
                if (cond3) { stmt; }  /// Moves
        }
    )", R"(
        void f() {
            if (cond1) { }
            if (cond3) { stmt; }      /// Moves
        }
    )", true);
}

TEST_CASE("Builtin type to user defined type is detected", "[comparison]")
{
    diffSources(R"(
        size_t       /// Updates
            f();
        void g(
            size_t   /// Updates
            param);
    )", R"(
        int          /// Updates
            f();
        void g(
            int      /// Updates
            param);
    )", false);
}

TEST_CASE("Identical non-interchangeable nodes are matched", "[comparison]")
{
    diffSources(R"(
        void f() {
            if(cond1) {
                int a;
            } else if(cond2)
                ;
        }
    )", R"(
        void f() {
            if(cond1) {
                int a;
                int b;        /// Additions
            } else if(cond2)
                ;
        }
    )", true);
}

TEST_CASE("Returns with and without value aren't matched",
          "[comparison][parsing]")
{
    diffSources(R"(
        void f() {       /// Deletions
            return 1;    /// Deletions
        }                /// Deletions
    )", R"(
        void f() {       /// Additions
            if (cond) {  /// Additions
                return;  /// Additions
            }            /// Additions
        }                /// Additions
    )", false);
}

TEST_CASE("Curly braces are considered part of if block",
          "[comparison][splicing]")
{
    diffSources(R"(
        void f()
        {
            if (cond) {
                int a;   /// Deletions
            }
        }
    )", R"(
        void f()
        {
            if (cond) {
            }
        }
    )", false);
}

TEST_CASE("Argument list is decomposed", "[comparison][parsing]")
{
    diffSources(R"(
        void f() {
            func(arg, arg,
                 structure->field    /// Deletions
            );
        }
    )", R"(
        void f() {
            func(arg, arg,
                 process(structure)  /// Additions
            );
        }
    )", false);
}

TEST_CASE("Assignment is decomposed", "[comparison][parsing]")
{
    diffSources(R"(
        void f() {
            var = func(arg, arg,
                       p->dir_field  /// Deletions
            );
        }
    )", R"(
        void f() {
            var = func(arg, arg,
                       dir           /// Additions
            );
        }
    )", false);
}

TEST_CASE("Nesting of calls is considered on comparison",
          "[comparison][parsing]")
{
    diffSources(R"(
        void f() {
            if (call(
                     val        /// Deletions
            )) {
                return;
            }
        }
    )", R"(
        void f() {
            if (call(
                     func(val)  /// Additions
            )) {
                return;
            }
        }
    )", false);
}

TEST_CASE("Removing enumeration element", "[comparison][parsing]")
{
    diffSources(R"(
        enum {
            UUE_FULL_RELOAD, /* Full view reload. */
        };
    )", R"(
        enum {
            UUE_FULL_RELOAD /* Full view reload. */
        };
    )", false);

    diffSources(R"(
        enum {
            UUE_REDRAW,
            UUE_RELOAD,      /* View reload. */
            UUE_FULL_RELOAD, /* Full view reload. */  /// Deletions
        };
    )", R"(
        enum {
            UUE_REDRAW,
            UUE_RELOAD,      /* View reload. */
        };
    )", false);

    diffSources(R"(
        enum {
            UUE_RELOAD,      /* View reload. */
            UUE_FULL_RELOAD, /* Full view reload. */  /// Deletions
        };
    )", R"(
        enum {
            UUE_RELOAD,      /* View reload. */
        };
    )", false);
}

TEST_CASE("Variable name isn't removed/added with initializer",
          "[comparison][parsing]")
{
    diffSources(R"(
        void f() {
            const char
                      *       /// Deletions
                      ext;
            ext = NULL;       /// Deletions
        }
    )", R"(
        void f() {
            const char
                      *const  /// Additions
                      ext
                      = NULL  /// Additions
                      ;
        }
    )", false);
}

TEST_CASE("Whitespace after newline in comments is ignored", "[comparison]")
{
    diffSources(R"(
        /* line1
         * line2 */
    )", R"(
          /* line1
           * line2 */
    )", false);
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

static void
diffSources(const std::string &left, const std::string &right, bool skipRefine)
{
    std::string cleanedLeft, cleanedRight;
    std::vector<Changes> expectedOld, expectedNew;
    std::tie(cleanedLeft, expectedOld) = extractExpectations(left);
    std::tie(cleanedRight, expectedNew) = extractExpectations(right);

    Tree oldTree = makeTree(cleanedLeft, true);
    Tree newTree = makeTree(cleanedRight, true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, skipRefine);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());
    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

static std::pair<std::string, std::vector<Changes>>
extractExpectations(const std::string &src)
{
    std::vector<std::string> lines = split(src, '\n');

    auto allSpaces = [](const std::string &str) {
        return (str.find_first_not_of(' ') == std::string::npos);
    };

    while (!lines.empty() && allSpaces(lines.back())) {
        lines.pop_back();
    }

    std::vector<Changes> changes;
    changes.reserve(lines.size());

    std::string cleanedSrc;
    cleanedSrc.reserve(src.length());

    for (const std::string &line : lines) {
        std::string src, expectation;
        std::tie(src, expectation) = splitAt(line, "/// ");

        cleanedSrc += src;
        cleanedSrc += '\n';

        if (expectation == "" || expectation == "No") {
            changes.push_back(Changes::No);
        } else if (expectation == "Additions") {
            changes.push_back(Changes::Additions);
        } else if (expectation == "Deletions") {
            changes.push_back(Changes::Deletions);
        } else if (expectation == "Updates") {
            changes.push_back(Changes::Updates);
        } else if (expectation == "Moves") {
            changes.push_back(Changes::Moves);
        } else if (expectation == "Mixed") {
            changes.push_back(Changes::Mixed);
        } else {
            REQUIRE_FALSE(true);
        }
    }

    return { cleanedSrc, changes };
}

/**
 * @brief Splits string in two parts at the leftmost delimiter.
 *
 * @param s String to split.
 * @param delim Delimiter, which separates left and right parts of the string.
 *
 * @returns Pair of left and right string parts.
 *
 * @throws std::runtime_error On failure to find delimiter in the string.
 */
static std::pair<std::string, std::string>
splitAt(const std::string &s, const std::string &delim)
{
    const std::string::size_type pos = s.find(delim);
    if (pos == std::string::npos) {
        return { s, std::string() };
    }

    return { s.substr(0, pos), s.substr(pos + delim.length()) };
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
            case State::Updated:  change = Changes::Updates; break;
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
        case Changes::Updates:   return (os << "Updates");
        case Changes::Moves:     return (os << "Moves");
        case Changes::Mixed:     return (os << "Mixed");
    }

    return (os << "Unknown Changes value");
}
