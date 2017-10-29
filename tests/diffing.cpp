#include "Catch/catch.hpp"

#include <functional>

#include "TreeBuilder.hpp"
#include "compare.hpp"
#include "time.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Reduced tree is compared correctly", "[comparison][reduction]")
{
    diffSources(R"(
        void f() {
            if (
                variable < 2 ||  /// Deletions
                condition        /// Moves
               )
                return;
        }
    )", R"(
        void f() {
            if (
                condition        /// Moves
                )
                return;
        }
    )", true);
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

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 0);
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
    // updated.
    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
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
              1                    /// Updates
            }, { .another_field =
              1                    /// Updates
            },
        };
    )", R"(
        aggregate var = {
            { .field =
              2                    /// Updates
            }, { .another_field =
              2                    /// Updates
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
            1        /// Updates
            ;
        };
    )", R"(
        struct s {
            int g;   /// Additions
            int b :
            2        /// Updates
            ;
        };
    )", false);

    diffSources(R"(
        struct S {
            int
                field1  /// Updates
                ;
        };
    )", R"(
        struct S {
            int
                field2  /// Updates
                ;
        };
    )", true);

    diffSources(R"(
        typedef struct {
            view_t left;   /// Deletions
            view_t right;  /// Deletions
        } tab_t;
    )", R"(
        typedef struct {
            view_t view;   /// Additions
        } tab_t;
    )", true);

    diffSources(R"(
        struct tab {
            view_t left;   /// Deletions
            view_t right;  /// Deletions
        };
    )", R"(
        struct tab {
            view_t view;   /// Additions
        };
    )", true);

    diffSources(R"(
        struct tab {
            view_t left;   /// Deletions
        };
    )", R"(
        struct tab { };
    )", true);

    diffSources(R"(
        struct {
            int left;      /// Deletions
        } instance;
    )", R"(
        struct { } instance;
    )", true);
}

TEST_CASE("Comments are ignored on high-level comparison", "[comparison]")
{
    diffSources(R"(
        typedef enum {
            NF_NONE,
            NF_ROOT,
            NF_FULL
        } NameFormat;
    )", R"(
        /* Defines the way entry name should be formatted. */  /// Additions
        typedef enum {
            /* No formatting at all. */                        /// Additions
            NF_NONE,
            /* Exclude extension and format the rest. */       /// Additions
            NF_ROOT,
            /* Format the whole name. */                       /// Additions
            NF_FULL
        } NameFormat;
    )", true);
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
            Aaaa,  /// Mixed
            Bbbb,  /// Mixed
        };
    )", R"(
        enum {
            A,
            B,
            Aaab,  /// Mixed
            Bbbz   /// Updates
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

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

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
            var
            ;
    )", R"(
        int
            var
            =   /// Additions
            10  /// Additions
            ;
    )", true);
}

TEST_CASE("Adding/removing/modifying initializers", "[comparison]")
{
    diffSources(R"(
        char ARR1[] =
                    "a"        /// Deletions
                    ;

        char ARR2[]
                    = "a"      /// Deletions
                    ;

        const char ARR3[];
    )", R"(
        char ARR1[] =
                    "b"        /// Additions
                    ;

        char ARR2[];

        const char ARR3[]
                        = "b"  /// Additions
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

TEST_CASE("Move detection works in a function", "[comparison][moves]")
{
    diffSources(R"(
        void f() {
            call1();  /// Moves
            call2();
            call3();
        }
    )", R"(
        void f() {
            call2();
            call3();
            call1();  /// Moves
        }
    )", true);

    diffSources(R"(
        void f() {
            int a;  /// Moves
            int b;
            int c;
        }
    )", R"(
        void f() {
            int b;
            int c;
            int a;  /// Moves
        }
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

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

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

TEST_CASE("Removing/adding block braces don't make single statement moved",
          "[comparison][moves]")
{
    diffSources(R"(
        void f() {
            if (1)
                call();
        }
    )", R"(
        void f() {
            if (1) {
                call();
            }
        }
    )", true);

    diffSources(R"(
        void f() {
            if (1) {
                call();
            }
        }
    )", R"(
        void f() {
            if (1)
                call();
        }
    )", true);
}

TEST_CASE("Moves in nested structures are detected meaningfully",
          "[comparison][moves]")
{
    diffSources(R"(
        void f() {
            if (cond1) {
                stmt0;        /// Moves
            }
        }
    )", R"(
        void f() {
            if (cond1) {
                if (cond2) {  /// Additions
                    stmt0;    /// Moves
                }             /// Additions
            }
        }
    )", true);
}

TEST_CASE("Builtin type to user defined type is detected", "[comparison]")
{
    diffSources(R"(
        size_t                 /// Updates
            func();
        void g(
            size_t             /// Updates
            long_param_name);
    )", R"(
        int                    /// Updates
            func();
        void g(
            int                /// Updates
            long_param_name);
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
            func(arg, arg
                 , structure->field    /// Deletions
            );
        }
    )", R"(
        void f() {
            func(arg, arg
                 , process(structure)  /// Additions
            );
        }
    )", false);
}

TEST_CASE("Comma is bundled to arguments", "[comparison][parsing]")
{
    diffSources(R"(
        void f() {
            call(arg1, arg3);
        }
    )", R"(
        void f() {
            call(arg1
                 , expr        /// Additions
                 , arg3);
        }
    )", false);
}

TEST_CASE("Parameter list of prototypes is decomposed", "[comparison][parsing]")
{
    diffSources(R"(
        void function(
            void        /// Deletions
        );
    )", R"(
        void function(
            int i       /// Additions
        );
    )", true);

    diffSources(R"(
        void func(const char str[]);
    )", R"(
        void func(const char str[]
                  ,                   /// Additions
                  int file_hi_only    /// Additions
                  );
    )", true);
}

TEST_CASE("Parameter list of definitions is decomposed",
          "[comparison][parsing]")
{
    diffSources(R"(
        void func(const char str[])
        {
        }
    )", R"(
        void func(
            int file_hi_only         /// Additions
            , const char str[])
        {
        }
    )", true);
}

TEST_CASE("Assignment is decomposed", "[comparison][parsing]")
{
    diffSources(R"(
        void f() {
            var = func(arg, arg
                       , p->dir_field  /// Deletions
            );
        }
    )", R"(
        void f() {
            var = func(arg, arg
                       , dir           /// Additions
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

TEST_CASE("Conditional expression is decomposed", "[comparison]")
{
    diffSources(R"(
        const int a = cond
                      ?
                      yes1  /// Updates
                      :
                      no
                      ;
    )", R"(
        const int a = cond
                      ?
                      yes2  /// Updates
                      :
                      no
                      ;
    )", true);
}

TEST_CASE("Return statement with value is decomposed", "[comparison]")
{
    diffSources(R"(
        void f() {
            return
                   cond ? view->list_pos >= view->run_size :  /// Deletions
                          view->list_pos > 0                  /// Moves
                          ;
        }
    )", R"(
        void f() {
            return
                   (                                          /// Additions
                       view->list_pos > 0                     /// Moves
                   )                                          /// Additions
                   ;
        }
    )", false);
}

TEST_CASE("If condition is matched separately from if-statement structure",
          "[comparison]")
{
    diffSources(R"(
        void f() {
            if (
                doit(1)
            )
            {
                stmt;     /// Moves
            }
        }
    )", R"(
        void f() {
            if (
                !         /// Additions
                doit(1)
            )
            {
                return;   /// Additions
            }

            stmt;         /// Moves
        }
    )", false);
}

TEST_CASE("Constants can be updated", "[comparison]")
{
    diffSources(R"(
        int i =
                1             /// Updates
                ;
        float f =
                  1.0f        /// Updates
                  ;
        char c =
                 'a'          /// Updates
                 ;
        char str[] =
                     "this"   /// Updates
                     ;
    )", R"(
        int i =
                2             /// Updates
                ;
        float f =
                  2.0f        /// Updates
                  ;
        char c =
                 'b'          /// Updates
                 ;
        char str[] =
                     "thus"   /// Updates
                     ;
    )", false);
}

TEST_CASE("Top-level declarations aren't mixed", "[comparison]")
{
    diffSources(R"(
        typedef struct {
            view_t left;                              /// Deletions
            view_t right;                             /// Deletions

            int active_win; // 0 -- left, 1 -- right  /// Deletions
            int only_mode;                            /// Deletions
            preview_t preview;
        } tab_t;

        typedef struct {                              /// Deletions
            tab_t *tabs;                              /// Deletions
            DA_INSTANCE(tabs);                        /// Deletions
            int current;                              /// Deletions
        } inner_tab_t;                                /// Deletions

        static
               tab_t                                  /// Updates
               *tabs;
    )", R"(
        typedef struct {
            view_t view;                              /// Additions

            preview_t preview;
        } tab_t;

        static inner_tab_t * get_inner_tab(const view_t *view);          /// Additions
        static int tabs_new_outer(void);                                 /// Additions
        static tab_t * tabs_new_inner(inner_tab_t *itab, view_t *view);  /// Additions
        static void tabs_goto_outer(int number);                         /// Additions

        static
               outer_tab_t                            /// Updates
               *tabs;
    )", false);
}

TEST_CASE("Return with value is on a separate layer", "[comparison]")
{
    diffSources(R"(
        void f() {
            return 0;
        }
    )", R"(
        void f() {
            for (i = 0; i < 1000; ++i) { }  /// Additions
            return 0;
        }
    )", false);
}

TEST_CASE("Translation unit is not moved", "[comparison]")
{
    diffSources(R"(
        int a;

        MACRO(      /// Deletions
            int b;  /// Moves
        )           /// Deletions
    )", R"(
        int a;

        int b;      /// Moves
    )", false);
}

TEST_CASE("Matching parent value guides how leaves are matched", "[comparison]")
{
    diffSources(R"(
        void f() {
            if (descr == NULL) {
                statement;        /// Moves
                return -1;
            }
        }
    )", R"(
        void f() {
            if (cond) {           /// Additions
                statement;        /// Moves
                thisIsNewStmt;    /// Additions
                return -1;        /// Additions
            }                     /// Additions

            if (descr == NULL) {
                return -1;
            }
        }
    )", false);

    diffSources(R"(
        void f() {
            if (descr == NULL) {
                statement;        /// Moves
                return -1;
            }
        }
    )", R"(
        void f() {
            if (cond) {           /// Additions
                statement;        /// Moves
                return -1;        /// Additions
            }                     /// Additions

            if (descr == NULL) {
                return -1;
            }
        }
    )", false);
}

TEST_CASE("Expressions are matched against if condition", "[comparison][moves]")
{
    diffSources(R"(
        void f() {
            magic_load(magic, NULL)      /// Moves
                ;                        /// Deletions
        }
    )", R"(
        void f() {
            if (                         /// Additions
                magic_load(magic, NULL)  /// Moves
                != 0)                    /// Additions
            { }                          /// Additions
        }
    )", false);

    diffSources(R"(
        void f() {
            tabs_new_inner(&         /// Moves
                           rwin      /// Updates
                           )         /// Moves
                           ;         /// Deletions
        }
    )", R"(
        void f() {
            if (                     /// Additions
                tabs_new_inner(&     /// Moves
                               lwin  /// Updates
                               )     /// Moves
                == NULL || 1) { }    /// Additions
        }
    )", true);

    diffSources(R"(
        void f() {
            tabs_new_inner(&         /// Moves
                           lwin      /// Moves
                           )         /// Moves
                           ;         /// Deletions
        }
    )", R"(
        void f() {
            if (                     /// Additions
                tabs_new_inner(&     /// Moves
                               lwin  /// Moves
                               )     /// Moves
                == NULL || 1) { }    /// Additions
        }
    )", true);

    diffSources(R"(
        void f() {
            tabs_new_inner(&new_tab->left, &lwin)       /// Moves
                ;                                       /// Deletions
            tabs_new_inner(&new_tab->right, &rwin)      /// Moves
                ;                                       /// Deletions
        }
    )", R"(
        void f() {
            if (                                        /// Additions
                tabs_new_inner(&new_tab->left, &lwin)   /// Moves
                == NULL ||                              /// Additions
                tabs_new_inner(&new_tab->right, &rwin)  /// Moves
                == NULL)                                /// Additions
            { }                                         /// Additions
        }
    )", true);
}

TEST_CASE("Refining doesn't cause segfault", "[comparison][moves]")
{
    diffSources(R"(
        void f() {
            &stats->
                    preview_cleanup  /// Updates
                    ;
        }
    )", R"(
        void f() {
            &stats->
                    preview          /// Updates
                    .cleanup_cmd     /// Additions
                    ;
        }
    )", false);
}

TEST_CASE("Postponed node in initializer is matched on adding elements",
          "[comparison][postponed]")
{
    diffSources(R"(
        const char *list[] = {
        #ifndef MACRO
            [SK_BY_NLINKS] = "nlinks",
            [SK_BY_INODE]  = "inode",
        #endif
        };
    )", R"(
        const char *list[] = {
        #ifndef MACRO
            [SK_BY_NLINKS] = "nlinks",
            [SK_BY_INODE]  = "inode",
        #endif

            [SK_BY_ID]     = ""         /// Additions
        };
    )", false);
}

TEST_CASE("Comma after initializer element is bundled with the element",
          "[comparison]")
{
    diffSources(R"(
        const char *list[] = {
            "nlinks",           /// Deletions
            "inode",
        };
    )", R"(
        const char *list[] = {
            "inode",
        };
    )", false);

    diffSources(R"(
        const char *list[] = {
            "long string value"
            ,                    /// Deletions
        };
    )", R"(
        const char *list[] = {
            "long string value"
        };
    )", false);
}

TEST_CASE("Parameter moves are detected", "[comparison][moves]")
{
    diffSources(R"(
        void f(
               int a    /// Moves
               ,
               float b
        );
    )", R"(
        void f(
               float b
               ,        /// Moves
               int a    /// Moves
        );
    )", false);
}

TEST_CASE("Parenthesised expression is decomposed", "[comparison][parsing]")
{
    diffSources(R"(
        int a = (
                 1002  /// Updates
                );
    )", R"(
        int a = (
                 1001  /// Updates
                );
    )", true);
}

TEST_CASE("Additive expression is decomposed", "[comparison][parsing]")
{
    diffSources(R"(
        int a = 101 +
                202    /// Updates
                ;
    )", R"(
        int a = 101 +
                203    /// Updates
                ;
    )", true);
}

TEST_CASE("Assignments aren't equal to each other", "[comparison][parsing]")
{
    diffSources(R"(
        void f() {
            var1
            =        /// Updates
            1; var2
            /=       /// Updates
            2; var3
            %=       /// Updates
            3; var4
            <<=      /// Updates
            4; var5
            &=       /// Updates
            5; var6
            |=       /// Updates
            6;
        }
    )", R"(
        void f() {
            var1
            +=       /// Updates
            1; var2
            *=       /// Updates
            2; var3
            -=       /// Updates
            3; var4
            >>=      /// Updates
            4; var5
            |=       /// Updates
            5; var6
            ^=       /// Updates
            6;
        }
    )", true);
}

TEST_CASE("For loop is decomposed", "[comparison][parsing]")
{
    diffSources(R"(
        void f() {
            for (
                 i = 0;
                 i < 1000;
                 ++i              /// Deletions
            ) { doSomething(); }
        }
    )", R"(
        void f() {
            for (
                 i = 0;
                 i < 1000;
            ) { doSomething(); }
        }
    )", true);

    diffSources(R"(
        void f() {
            for (
                 i = 0;
                 i < 1000         /// Deletions
                 ; ++i
            ) { doSomething(); }
        }
    )", R"(
        void f() {
            for (
                 i = 0;
                 ;
                 ++i
            ) { doSomething(); }
        }
    )", true);

    diffSources(R"(
        void f() {
            for (
                 i = 0            /// Deletions
                 ; i < 1000;
                 ++i
            ) { doSomething(); }
        }
    )", R"(
        void f() {
            for (
                 ; i < 1000;
                 ++i
            ) { doSomething(); }
        }
    )", true);

    diffSources(R"(
        void f() {
            for (
                 i = 0            /// Moves
                 ; ;
            ) { doSomething(); }
        }
    )", R"(
        void f() {
            for (;
                 i = 0            /// Moves
                 ;
            ) { doSomething(); }
        }
    )", true);

    diffSources(R"(
        void f() {
            for (
                 i = 0            /// Moves
                 ; ;
            ) { doSomething(); }
        }
    )", R"(
        void f() {
            for (; ;
                 i = 0            /// Moves
            ) { doSomething(); }
        }
    )", true);
}

TEST_CASE("For loop header is on a separate layer", "[comparison]")
{
    diffSources(R"(
        void f() {
            for (i = 0; i < cs->file_hi_count; ++i) {             /// Deletions
                add_match(expr, "");                              /// Deletions
            }                                                     /// Deletions

            if (!file_hi_only) {                                  /// Deletions
                for (i = 0; i < MAXNUM_COLOR; ++i) {              /// Moves
                    add_match(HI_GROUPS[i], HI_GROUPS_DESCR[i]);  /// Moves
                }                                                 /// Moves
            }                                                     /// Deletions
        }
    )", R"(
        void f() {
            for (i = 0; i < MAXNUM_COLOR; ++i) {                  /// Moves
                add_match(HI_GROUPS[i], HI_GROUPS_DESCR[i]);      /// Moves
            }                                                     /// Moves
        }
    )", true);
}

TEST_CASE("Prefix increment/decrement is decomposed", "[comparison][parsing]")
{
    diffSources(R"(
        void f() {
            ++         /// Updates
            variable;
        }
    )", R"(
        void f() {
            --         /// Updates
            variable;
        }
    )", true);
}

TEST_CASE("Widely different comments aren't matched", "[comparison]")
{
    diffSources(R"(
        //          /// Deletions
    )", R"(
        /* Bla. */  /// Additions
    )", true);
}

TEST_CASE("Widely different directives aren't matched", "[comparison]")
{
    diffSources(R"(
        #if 0                    /// Deletions
        #endif                   /// Deletions
    )", R"(
        #include "cfg/config.h"  /// Additions
    )", true);
}

TEST_CASE("Logical operators are a separate category", "[comparison]")
{
    diffSources(R"(
        int a = 1
             &&     /// Deletions
                2;
    )", R"(
        int a = 1
              *     /// Additions
                2;
    )", false);

    diffSources(R"(
        int a = 1
             &&     /// Updates
                2;
    )", R"(
        int a = 1
             ||     /// Updates
                2;
    )", false);
}

TEST_CASE("Comparison operator change is detected as update",
          "[comparison][parsing]")
{
    diffSources(R"(
        int a = variable
                ==        /// Updates
                value;
    )", R"(
        int a = variable
                !=        /// Updates
                value;
    )", true);

    diffSources(R"(
        int a = variable
                ==        /// Updates
                value;
    )", R"(
        int a = variable
                <=        /// Updates
                value;
    )", true);
}

TEST_CASE("Negation is decomposed", "[comparison][parsing]")
{
    /* The code is relatively complex, because it works otherwise. */
    diffSources(R"(
        void f() {
            if (var == 0 &&
                !                                    /// Deletions
                curr_stats.restart_in_progress) { }
        }
    )", R"(
        void f() {
            if (var == 0 && curr_stats.restart_in_progress) { }
        }
    )", true);
}

TEST_CASE("Inversion of relatively complex condition", "[comparison][parsing]")
{
    diffSources(R"(
        void f() {
            if (var
                ==                                   /// Updates
                0
                &&                                   /// Updates
                !                                    /// Deletions
                curr_stats.restart_in_progress) { }
        }
    )", R"(
        void f() {
            if (var
                !=                                   /// Updates
                0
                ||                                   /// Updates
                curr_stats.restart_in_progress) { }
        }
    )", true);
}
