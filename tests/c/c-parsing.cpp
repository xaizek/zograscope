// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

// These are tests of more advanced properties of a parser, which are much
// easier and reliable to test by performing comparison.

#include "Catch/catch.hpp"

#include "utils/time.hpp"
#include "compare.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Function specifiers are detected as such", "[comparison][parsing]")
{
    Tree oldTree = parseC("TYPEMACRO void f();", true);
    Tree newTree = parseC("void f();", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 1);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 0);
}

TEST_CASE("Flat initializer is decomposed", "[comparison][parsing]")
{
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    )", true);

    diffC(R"(
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

    diffC(R"(
        typedef struct {
            view_t left;   /// Deletions
            view_t right;  /// Deletions
        } tab_t;
    )", R"(
        typedef struct {
            view_t view;   /// Additions
        } tab_t;
    )", true);

    diffC(R"(
        struct tab {
            view_t left;   /// Deletions
            view_t right;  /// Deletions
        };
    )", R"(
        struct tab {
            view_t view;   /// Additions
        };
    )", true);

    diffC(R"(
        struct tab {
            view_t left;   /// Deletions
        };
    )", R"(
        struct tab { };
    )", true);

    diffC(R"(
        struct {
            int left;      /// Deletions
        } instance;
    )", R"(
        struct { } instance;
    )", true);
}

TEST_CASE("Structure with one element is decomposed", "[comparison][parsing]")
{
    diffC(R"(
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
    diffC(R"(
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
    Tree oldTree = parseC(R"(
        void func()
        {
            /* Comment. */
            return;
        }
    )", true);
    Tree newTree = parseC(R"(
        void func()
        {
            char array[1];
            /* Comment. */
            return;
        }
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) == 1);
}

TEST_CASE("Coarse nodes are formed correctly", "[comparison][parsing]")
{
    Tree oldTree = parseC(R"(
        int var;
    )", true);
    Tree newTree = parseC(R"(
        // Comment.
        int var;
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

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
    Tree oldTree = parseC(R"(
        void f() {
            if(condition1) {
                if(condition2) {
                }
            }
        }
    )", true);
    Tree newTree = parseC(R"(
        void f() {
            if(condition1) {
                // comment
                if(condition2) {
                }
            }
        }
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

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
    Tree oldTree = parseC(R"(
        void f() {
            int a = {};
        }
    )", true);
    Tree newTree = parseC(R"(
        void f() {
            int a;
        }
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    CHECK(countLeaves(*oldTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Deleted) > 0);
    CHECK(countLeaves(*oldTree.getRoot(), State::Inserted) == 0);

    CHECK(countLeaves(*newTree.getRoot(), State::Updated) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Deleted) == 0);
    CHECK(countLeaves(*newTree.getRoot(), State::Inserted) > 0);
}

TEST_CASE("Returns with and without value aren't matched",
          "[comparison][parsing]")
{
    diffC(R"(
        void f() {
            return 1;    /// Deletions
        }
    )", R"(
        void f() {
            if (cond) {  /// Additions
                return;  /// Additions
            }            /// Additions
        }
    )", false);
}

TEST_CASE("Argument list is decomposed", "[comparison][parsing]")
{
    diffC(R"(
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

    diffC(R"(
        void f() {
            fprintf(fp, "%s\n",
                    hist->items[i]       /// Deletions
            );
        }
    )", R"(
        void f() {
            fprintf(fp, "%s\n",
                    hist->items[i].text  /// Additions
            );
        }
    )", false);

    diffC(R"(
        void f() {
            set_colorscheme(
                cmd_info->argv[0]  /// Deletions
            );
        }
    )", R"(
        void f() {
            set_colorscheme(
                cmd_info->argv     /// Additions
            );
        }
    )", false);

    diffC(R"(
        void f() {
            function(firstArg);
        }
    )", R"(
        void f() {
            function(
                     newArg        /// Additions
                     , firstArg);
        }
    )", true);
}

TEST_CASE("Comma is bundled to arguments", "[comparison][parsing]")
{
    diffC(R"(
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
    diffC(R"(
        void function(
            void        /// Deletions
        );
    )", R"(
        void function(
            int i       /// Additions
        );
    )", true);

    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
        enum {
            UUE_FULL_RELOAD, /* Full view reload. */
        };
    )", R"(
        enum {
            UUE_FULL_RELOAD /* Full view reload. */
        };
    )", false);

    diffC(R"(
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

    diffC(R"(
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
    diffC(R"(
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

TEST_CASE("Parenthesised expression is decomposed", "[comparison][parsing]")
{
    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
        void f() {
            vvar1
            =         /// Updates
            1; vvar2
            /=        /// Updates
            2; vvar3
            %=        /// Updates
            3; vvar4
            <<=       /// Updates
            4; vvar5
            &=        /// Updates
            5; vvar6
            |=        /// Updates
            6;
        }
    )", R"(
        void f() {
            vvar1
            +=        /// Updates
            1; vvar2
            *=        /// Updates
            2; vvar3
            -=        /// Updates
            3; vvar4
            >>=       /// Updates
            4; vvar5
            |=        /// Updates
            5; vvar6
            ^=        /// Updates
            6;
        }
    )", true);
}

TEST_CASE("For loop is decomposed", "[comparison][parsing]")
{
    diffC(R"(
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

    diffC(R"(
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

    diffC(R"(
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

    diffC(R"(
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

    diffC(R"(
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

    diffC(R"(
        void f() {
            for (;;) { }
        }
    )", R"(
        void f() {
            for (
                i = 0     /// Additions
                ;;) {
            }
        }
    )", true);

    diffC(R"(
        void f() {
            for (int i = 0;
                 i < 0       /// Deletions
                 ;
                 --i         /// Moves
                 ) {
            }
        }
    )", R"(
        void f() {
            for (int i = 0;
                 --i         /// Moves
                 ; ) {
            }
        }
    )", true);

    diffC(R"(
        void f() {
            for (int i = 0; ; ) { }
        }
    )", R"(
        void f() {
            for (int i = 0; ;
                 ++i                 /// Additions
                ) {
            }
        }
    )", true);
}

TEST_CASE("Moves in statements with fixed structure",
          "[comparison][parsing][postponed]")
{
    diffC(R"(
        void f() {
            for (
                 i = 0;
                 i < 1000;
                 ++i
                 // Comment1      /// Moves
                 // Comment2
            ) { doSomething(); }
        }
    )", R"(
        void f() {
            for (
                 i = 0;
                 i < 1000;
                 ++i
                 // new           /// Additions
                 // Comment2
                 // Comment1      /// Moves
            ) { doSomething(); }
        }
    )", true);
}

TEST_CASE("Prefix increment/decrement is decomposed", "[comparison][parsing]")
{
    diffC(R"(
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

TEST_CASE("Comparison operator change is detected as update",
          "[comparison][parsing]")
{
    diffC(R"(
        int a = variable
                ==        /// Updates
                value;
    )", R"(
        int a = variable
                !=        /// Updates
                value;
    )", true);

    diffC(R"(
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
    diffC(R"(
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
    diffC(R"(
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

TEST_CASE("Reordering of conditionals", "[comparison][parsing]")
{
    diffC(R"(
        void f() {
            if (
                cfg_confirm_delete(0)      /// Moves
                &&
                !curr_stats.confirmed
                &&                         /// Moves
                (ops == NULL || !ops->bg)  /// Moves
            ) {
            }
        }
    )", R"(
        void f() {
            if (
                (ops == NULL || !ops->bg)  /// Moves
                &&
                cfg_confirm_delete(0)      /// Moves
                &&                         /// Moves
                !curr_stats.confirmed
            ) {
            }
        }
    )", true);
}

TEST_CASE("Condition replacement", "[comparison][parsing]")
{
    diffC(R"(
        void f() {
            if (cfg.use_system_calls
                &&
                !is_dir(fname)                      /// Deletions
                &&
                !is_dir(caused_by)                  /// Deletions
            ) {
                responses[i++] = append;
            }
        }
    )", R"(
        void f() {
            if (cfg.use_system_calls
                &&
                is_regular_file_noderef(fname)      /// Additions
                &&
                is_regular_file_noderef(caused_by)  /// Additions
            ) {
                responses[i++] = append;
            }
        }
    )", true);
}

TEST_CASE("Switch and label statements are decomposed", "[comparison][parsing]")
{
    diffC(R"(
        void f() {
            switch (v) {
                default:
                    doSomething();
                    break;
                case 1:               /// Moves
                    call(arg1         /// Moves
                             , arg2   /// Deletions
                             );       /// Moves
            }
        }
    )", R"(
        void f() {
            switch (v) {
                case 1:               /// Moves
                    call(arg1         /// Moves
                             , 10     /// Additions
                             );       /// Moves
                default:
                    doSomething();
                    break;
            }
        }
    )", true);
}

TEST_CASE("Do-while loop is decomposed", "[comparison][parsing]")
{
    diffC(R"(
        void f() {
            do
                step();
            while (condition);
        }
    )", R"(
        void f() {
            do {
                step();
                newCall();        /// Additions
            } while (condition);
        }
    )", true);
}

TEST_CASE("Call without arguments is decomposed", "[comparison][parsing]")
{
    diffC(R"(
        trashes_list list = get_list_of_trashes();
    )", R"(
        trashes_list list = get_list_of_trashes(
                                                0   /// Additions
                                                );
    )", true);
}

TEST_CASE("Sizeof of equivalent forms handled appropriately",
          "[comparison][parsing]")
{
    diffC(R"(
        int a = sizeof
                     (    /// Deletions
                     var  /// Moves
                     )    /// Deletions
                     ;
    )", R"(
        int a = sizeof
                     var  /// Moves
                     ;
    )", true);
}

TEST_CASE("Cast operator is decomposed", "[comparison][parsing]")
{
    diffC(R"(
        void f() {
            (void)                                                 /// Deletions
            format_and_send(ipc, from, data, EVAL_ERROR_TYPE)      /// Moves
            ;                                                      /// Deletions
        }
    )", R"(
        void f() {
            if (                                                   /// Additions
                format_and_send(ipc, from, data, EVAL_ERROR_TYPE)  /// Moves
                ) {                                                /// Additions
            }                                                      /// Additions
        }
    )", true);

    diffC(R"(
        void f() {
            fclose(dst, morearg, onemorearg);
        }
    )", R"(
        void f() {
            (void)                             /// Additions
            fclose(dst, morearg, onemorearg);
        }
    )", true);
}

TEST_CASE("Parameter list of macro function definition is decomposed",
          "[comparison][parsing]")
{
    diffC(R"(
        TEST(remote_expr_is_parsed) {
        }
    )", R"(
        TEST(remote_expr_is_parsed
             , IF(with_remote_cmds)  /// Additions
             ) {
        }
    )", true);
}

TEST_CASE("switch case labels don't introduce nested statements nodes",
          "[comparison][parsing]")
{
    diffC(R"(
        void f() {
            switch (bla) {
                case 1:
                    something();
                    something_else();
                case 2:                /// Moves
                    break;             /// Deletions
            }
        }
    )", R"(
        void f() {
            switch (bla) {
                case 1:
                case 2:                /// Moves
                    something();
                    something_else();
            }
        }
    )", true);
}

TEST_CASE("extern C is decomposed", "[comparison][parsing]")
{
    diffC(R"(
        extern "C" {
            int a;     /// Deletions
        }
    )", R"(
        extern "C" {
            void f();  /// Additions
        }
    )", true);
}

TEST_CASE("Function declaration with trailing macro is decomposed",
          "[comparison][parsing]")
{
    diffC(R"(
        void f(int arg1, int
                         arg2  /// Updates
              ) macro;
    )", R"(
        void f(int arg1, int
                         arg3  /// Updates
              ) macro;
    )", true);
}

TEST_CASE("Meta function declaration is decomposed", "[comparison][parsing]")
{
    diffC(R"(
        int macro(funcname, (int arg1, int
                             arg2           /// Updates
                            ));
    )", R"(
        int macro(funcname, (int arg1, int
                             arg3           /// Updates
                             ));
    )", true);
}

TEST_CASE("Blocks are spliced into for statements", "[comparison][parsing]")
{
    diffC(R"(
        void f() {
            for (i = 0; i < put_confirm.put.nitems; ++i) {
                pos = fpos_find_by_name(put_confirm.view);  /// Deletions
            }
        }
    )", R"(
        void f() {
            for (i = 0; i < put_confirm.put.nitems; ++i) {
                fpos_set_pos(put_confirm.view,              /// Additions
                             entry_to_pos(view, entry));    /// Additions
            }
        }
    )", true);
}

TEST_CASE("Multiply operator preserves its subexprs", "[comparison][parsing]")
{
    diffC(R"(
        void f() {
            int var = 2 +
                      2
                      /3  /// Updates
                      ;
        }
    )", R"(
        void f() {
            int var = 2 +
                      2
                      *8  /// Updates
                      ;
        }
    )", true);
}
