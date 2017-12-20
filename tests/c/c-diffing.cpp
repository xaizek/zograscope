// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

// These are tests of comparison and all of its phases.

#include "Catch/catch.hpp"

#include <functional>

#include "utils/time.hpp"
#include "compare.hpp"
#include "stypes.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Reduced tree is compared correctly", "[comparison][reduction]")
{
    diffC(R"(
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
    Tree oldTree = parseC(R"(
        aggregate var = {
            { .field = 1 },
        };
    )", true);
    Tree newTree = parseC(R"(
        aggregate var = {
            { .field = 2 },
        };
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 0);
}

TEST_CASE("Spaces are ignored during comparsion", "[comparison]")
{
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    Tree oldTree = parseC(R"(
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
    Tree newTree = parseC(R"(
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
    compare(oldTree, newTree, tr, true, false);

    // If functions matched correctly, only one leaf of the old tree will be
    // updated.
    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);
}

TEST_CASE("Comments are ignored on high-level comparison", "[comparison]")
{
    diffC(R"(
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

TEST_CASE("Declarations with and without initializer are not the same",
          "[comparison]")
{
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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

    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
        #include <stdio.h>
    )", R"(
        #include <stdio.h>
        long_string_of_some_words_to_throw_off_comparison var;  /// Additions
    )", true);
}

TEST_CASE("Unmoved statement is detected as such", "[comparison][moves]")
{
    diffC(R"(
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
    Tree oldTree = parseC(R"(
        // Comment.
        struct s {
            gid_t gid;
        };
    )", true);
    Tree newTree = parseC(R"(
        // Comment.
        struct s {
            id_t gid;
        };
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 1);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 0);
}

TEST_CASE("Else branch addition", "[comparison]")
{
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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

    diffC(R"(
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

    diffC(R"(
        void f() {
            while (true)
                doSomething();
        }
    )", R"(
        void f() {
            while (true) {
                doSomething();
            }
        }
    )", true);

    diffC(R"(
        void f() {
            switch (true)
                doSomething();
        }
    )", R"(
        void f() {
            switch (true) {
                doSomething();
            }
        }
    )", true);
}

TEST_CASE("Moves in nested structures are detected meaningfully",
          "[comparison][moves]")
{
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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

TEST_CASE("Curly braces are considered part of if block",
          "[comparison][splicing]")
{
    diffC(R"(
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

TEST_CASE("Whitespace after newline in comments is ignored", "[comparison]")
{
    diffC(R"(
        /* line1
         * line2 */
    )", R"(
          /* line1
           * line2 */
    )", false);
}

TEST_CASE("Conditional expression is decomposed", "[comparison]")
{
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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

    diffC(R"(
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
    diffC(R"(
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

    diffC(R"(
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

    diffC(R"(
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

    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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

    diffC(R"(
        const char *list[] = {
        #include "xmacro-file.h"
        };
    )", R"(
        const char *list[] = {
        #include "xmacro-file.h"
            extra                 /// Additions
        };
    )", true);

    diffC(R"(
        const char *list[] = {
        #include "xmacro-file.h"
        };
    )", R"(
        const char *list[] = {
            extra                 /// Additions
        #include "xmacro-file.h"
        };
    )", true);
}

TEST_CASE("Postponed nodes aren't marked moved on falling out of a container",
          "[comparison][postponed]")
{
    diffC(R"(
        void f() {
            movedStatement;    /// Moves
            // comment
            unmovedStatement;
        }
    )", R"(
        void f() {
            // comment
            unmovedStatement;
            movedStatement;    /// Moves
        }
    )", true);

    diffC(R"(
        void f() {
            movedStatement;    /// Moves
        #include "file.h"
            unmovedStatement;
        }
    )", R"(
        void f() {
        #include "file.h"
            unmovedStatement;
            movedStatement;    /// Moves
        }
    )", true);

    diffC(R"(
        void f() {
            movedStatement;    /// Moves
            // comment1
            // comment2
            unmovedStatement;
        }
    )", R"(
        void f() {
            // comment1
            // comment2
            unmovedStatement;
            movedStatement;    /// Moves
        }
    )", true);

    diffC(R"(
        void f() {
            movedStatement;    /// Moves
            // comment1
            unmovedStatement;
            // comment2
        }
    )", R"(
        void f() {
            // comment1
            unmovedStatement;
            // comment2
            movedStatement;    /// Moves
        }
    )", true);

    diffC(R"(
        void f() {
            #include "a"
        }
    )", R"(
        void f() {
            thisIsAMovedStatement;  /// Additions
            #include "a"
        }
    )", true);
}

TEST_CASE("Postponed nodes in structures", "[comparison][postponed]")
{
    diffC(R"(
        struct {
            int bla; // comment
        };
    )", R"(
        struct {
            int bla; // comment
            int added;           /// Additions
        };
    )", true);

    diffC(R"(
        struct name {
            int bla; // comment
        };
    )", R"(
        struct name {
            int bla; // comment
            int added;           /// Additions
        };
    )", true);

    diffC(R"(
        struct {
            // comment
        };
    )", R"(
        struct {
            // comment
            int v;      /// Additions
        };
    )", true);

    diffC(R"(
        struct aReallyLongName {
            // comment
        };
    )", R"(
        struct aReallyLongName {
            // comment
            int allow_empty;      /// Additions
        };
    )", true);
}

TEST_CASE("Comma after initializer element is bundled with the element",
          "[comparison]")
{
    diffC(R"(
        const char *list[] = {
            "nlinks",           /// Deletions
            "inode",
        };
    )", R"(
        const char *list[] = {
            "inode",
        };
    )", false);

    diffC(R"(
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
    diffC(R"(
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

TEST_CASE("For loop header is on a separate layer", "[comparison]")
{
    diffC(R"(
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

TEST_CASE("Widely different comments aren't matched", "[comparison]")
{
    diffC(R"(
        //          /// Deletions
    )", R"(
        /* Bla. */  /// Additions
    )", true);
}

TEST_CASE("Widely different directives aren't matched", "[comparison]")
{
    diffC(R"(
        #if 0                    /// Deletions
        #endif                   /// Deletions
    )", R"(
        #include "cfg/config.h"  /// Additions
    )", true);
}

TEST_CASE("Logical operators are a separate category", "[comparison]")
{
    diffC(R"(
        int a = 1
             &&     /// Deletions
                2;
    )", R"(
        int a = 1
              *     /// Additions
                2;
    )", false);

    diffC(R"(
        int a = 1
             &&     /// Updates
                2;
    )", R"(
        int a = 1
             ||     /// Updates
                2;
    )", false);
}

TEST_CASE("Somewhat complex expression changes", "[comparison]")
{
    diffC(R"(
        int primary =
                    (                                      /// Deletions
                    id == SK_BY_NAME || id == SK_BY_INAME  /// Moves
                    )                                      /// Deletions
                    ;
    )", R"(
        int primary =
                    id == SK_BY_NAME || id == SK_BY_INAME  /// Moves
                   || id == SK_BY_ROOT                     /// Additions
                   ;
    )", true);
}

TEST_CASE("Almost matched if-statement is matched till the end", "[comparison]")
{
    diffC(R"(
        void f() {
            if (msg == NULL ||
                vle_mode_is(CMDLINE_MODE)  /// Deletions
                )
            { return; }
        }
    )", R"(
        void f() {
            if (msg == NULL ||
                is_locked                  /// Additions
                )
            { return; }
        }
    )", true);
}

TEST_CASE("Function bodies are matched even when headers don't", "[comparison]")
{
    diffC(R"(
        TEST(path_is_invalidated_in_fsdata)                  /// Deletions
        {
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
        }
    )", R"(
        TEST(parents_are_mapped_in_fsdata_on_match)          /// Additions
        {
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
            fsdata_get(fsd, SANDBOX_PATH, &ch, sizeof(ch));
        }
    )", true);
}

TEST_CASE("Nodes with zero common non-satellite leaves are not marked updated",
          "[comparison]")
{
    // Internal node containing `default:` was marked as updated.

    Tree oldTree = parseC(R"(
        void f() {
            switch (v) {
                default:
                    doSomething();
                    break;
                case 1:
                    call(arg1, arg2);
            }
        }
    )", true);
    Tree newTree = parseC(R"(
        void f() {
            switch (v) {
                case 1:
                    call(arg1, 10);
                default:
                    doSomething();
                    break;
            }
        }
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    CHECK(countInternal(*oldTree.getRoot(), SType::LabelStmt, State::Updated)
          == 0);
    CHECK(countInternal(*newTree.getRoot(), SType::LabelStmt, State::Updated)
          == 0);
}

TEST_CASE("Head of while-loop is treated correctly", "[comparison]")
{
    diffC(R"(
        void f() {
            while (new_jobs != NULL) {
                new_job->err_next = *jobs;
                *jobs = new_job;
            }

            while (*job != NULL) {
                bg_job_t *const j = *job;
            }
        }
    )", R"(
        void f() {
            while (*job != NULL) {          /// Additions
            }                               /// Additions

            while (new_jobs != NULL) {
                new_job->err_next = *jobs;
                *jobs = new_job;
            }

            while (*job != NULL) {
                bg_job_t *const j = *job;
            }
        }
    )", true);
}

TEST_CASE("Statements are matched by common bodies", "[comparison]")
{
    diffC(R"(
        void f() {
            if (
                !at_first_line(view)                               /// Deletions
                ) {
                new_pos = MIN(new_pos, view->list_pos - (int)offset);
            }
        }
    )", R"(
        void f() {
            if (
                fpos_can_move_up(view)                             /// Additions
                ) {
                new_pos = MIN(new_pos, view->list_pos - (int)offset);
            }
        }
    )", true);

    diffC(R"(
        void f() {
            while (
                   new_jobs != NULL                    /// Deletions
                   ) {
                int new_pos;
                size_t offset = view->window_cells/2;
            }
        }
    )", R"(
        void f() {
            while (
                   veryDifferentCondition              /// Additions
                   ) {
                int new_pos;
                size_t offset = view->window_cells/2;
            }
        }
    )", true);
}

TEST_CASE("Simple expressions are matched consistently", "[comparison]")
{
    diffC(R"(
        void f() {
            !
             to_tree      /// Updates
             ;
            if (!
                 to_tree  /// Updates
                 );
        }
    )", R"(
        void f() {
            !
             as_tree      /// Updates
             ;
            if (!
                 as_tree  /// Updates
                 );
        }
    )", true);
}

TEST_CASE("Expressions aren't mixed with each other", "[comparison]")
{
    diffC(R"(
        void f() {
            while(new_jobs != NULL) {
                bg_job_t *const new_job = new_jobs;
            }

            while(*job != NULL) {
                bg_job_t *const j = *job;

                if(!j->running) {                          /// Deletions
                    *job = j->err_next;                    /// Deletions
                    pthread_spin_unlock(&j->status_lock);  /// Deletions
                    continue;                              /// Deletions
                }                                          /// Deletions
            }
        }
    )", R"(
        void f() {
            while(*job != NULL) {                          /// Additions
            }                                              /// Additions

            while(new_jobs != NULL) {
                bg_job_t *const new_job = new_jobs;
            }

            while(*job != NULL) {
                bg_job_t *const j = *job;
            }
        }
    )", true);

    diffC(R"(
        void f() {
            list.ntrashes =                                   /// Deletions
                add_to_string_array(&list.trashes,            /// Deletions
                                    list.ntrashes, 1, spec);  /// Deletions
        }
    )", R"(
        void f() {
            add_trash_to_list(&list, spec);                   /// Additions
        }
    )", true);

    diffC(R"(
        void f() {
            dcache_get_of(entry,
                          size               /// Updates
                          ,
                          nitems             /// Updates
                          );

            if(                              /// Moves
               *nitems                       /// Deletions
               == DCACHE_UNKNOWN && !        /// Moves
               view->on_slow_fs              /// Deletions
               ) {                           /// Moves
                *nitems                      /// Deletions
                = entry_calc_nitems(entry);  /// Moves
            }                                /// Moves
        }
    )", R"(
        void f() {
            const int is_slow_fs = view->on_slow_fs;  /// Additions

            dcache_result_t size_res, nitems_res;     /// Additions
            dcache_get_of(entry,
                          &                           /// Additions
                          size_res                    /// Updates
                          ,
                          &                           /// Additions
                          nitems_res                  /// Updates
                          );

            assert((size != NULL || nitems != NULL) &&                      /// Additions
                    "At least one of out parameters has to be non-NULL.");  /// Additions

            if(size != NULL) {                                              /// Additions
                *size = size_res.value;                                     /// Additions
                if(size_res.value != DCACHE_UNKNOWN &&                      /// Additions
                   !size_res.is_valid && !is_slow_fs) {                     /// Additions
                    *size = recalc_entry_size(entry, size_res.value);       /// Additions
                }                                                           /// Additions
            }                                                               /// Additions

            if(nitems != NULL) {                                            /// Additions
                if(                                                         /// Moves
                   nitems_res.value                                         /// Additions
                   == DCACHE_UNKNOWN && !                                   /// Moves
                   is_slow_fs                                               /// Additions
                   ) {                                                      /// Moves
                    nitems_res.value                                        /// Additions
                        = entry_calc_nitems(entry);                         /// Moves
                }                                                           /// Moves

                *nitems = (nitems_res.value == DCACHE_UNKNOWN ?             /// Additions
                           0 : nitems_res.value);                           /// Additions
            }                                                               /// Additions
        }
    )", true);
}

TEST_CASE("Refining works for fine trees", "[comparison]")
{
    Tree oldTree = parseC(R"(
        format_str("...%s", str);
    )");
    Tree newTree = parseC(R"(
        format_str("%s%s", ell, str);
    )");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, false);

    // Just checking that it doesn't crash.
}

TEST_CASE("Coarse reducing trees with and without layer breaks works",
          "[comparison]")
{
    Tree oldTree = parseC(R"(
        void f() {}
    )");
    Tree newTree = parseC(R"(
    )");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, false);

    // Just checking that it doesn't crash.
}

TEST_CASE("Nested statements with values are matched correctly", "[comparison]")
{
    diffC(R"(
        void g() {
            if(strcmp(part, "normal") == 0)         new_value |= SF_NORMAL;
            else if(strcmp(part, "visual") == 0)    new_value |= SF_VISUAL;
            else if(strcmp(part, "view") == 0)      new_value |= SF_VIEW;           /// Moves
            else if(strcmp(part, "otherpane") == 0) new_value |= SF_OTHERPANE;      /// Moves
            else if(strcmp(part, "keys") == 0)      new_value |= SF_KEYS;           /// Moves
            else if(strcmp(part, "marks") == 0)     new_value |= SF_MARKS;          /// Moves
            else if(starts_with_lit(part, "registers:")) {                          /// Moves
                const char *const num = after_first(part, ':');                     /// Moves
                new_value |= SF_REGISTERS;                                          /// Moves
            } else if(strcmp(part, "delay") == 0)     new_value |= SF_DELAY;        /// Moves
            else if(strcmp(part, "registers") == 0) new_value |= SF_REGISTERS;      /// Moves
            else {                                                                  /// Moves
                break_at(part, ':');                                                /// Moves
                break;                                                              /// Moves
            }                                                                       /// Moves
        }
    )", R"(
        void g() {
            if(strcmp(part, "normal") == 0)           new_value |= SF_NORMAL;
            else if(strcmp(part, "visual") == 0)      new_value |= SF_VISUAL;
            else if(strcmp(part, "foldsubkeys") == 0) new_value |= SF_FOLDSUBKEYS;  /// Additions
            else if(strcmp(part, "view") == 0)        new_value |= SF_VIEW;         /// Moves
            else if(strcmp(part, "otherpane") == 0)   new_value |= SF_OTHERPANE;    /// Moves
            else if(strcmp(part, "keys") == 0)        new_value |= SF_KEYS;         /// Moves
            else if(strcmp(part, "marks") == 0)       new_value |= SF_MARKS;        /// Moves
            else if(starts_with_lit(part, "registers:")) {                          /// Moves
                const char *const num = after_first(part, ':');                     /// Moves
                new_value |= SF_REGISTERS;                                          /// Moves
            } else if(strcmp(part, "delay") == 0)     new_value |= SF_DELAY;        /// Moves
            else if(strcmp(part, "registers") == 0) new_value |= SF_REGISTERS;      /// Moves
            else {                                                                  /// Moves
                break_at(part, ':');                                                /// Moves
                break;                                                              /// Moves
            }                                                                       /// Moves
        }
    )", true);
}
