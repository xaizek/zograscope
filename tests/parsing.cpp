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

// These are tests of more advanced properties of a parser, which are much
// easier and reliable to test by performing comparison.

#include "Catch/catch.hpp"

#include "utils/time.hpp"
#include "compare.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Function specifiers are detected as such", "[comparison][parsing]")
{
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
    )", true);

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

TEST_CASE("Structure with one element is decomposed", "[comparison][parsing]")
{
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

TEST_CASE("Returns with and without value aren't matched",
          "[comparison][parsing]")
{
    diffSources(R"(
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

    diffSources(R"(
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

    diffSources(R"(
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

    diffSources(R"(
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

    diffSources(R"(
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
    diffSources(R"(
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

TEST_CASE("Switch and label statements are decomposed", "[comparison][parsing]")
{
    diffSources(R"(
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
    diffSources(R"(
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
    diffSources(R"(
        trashes_list list = get_list_of_trashes();
    )", R"(
        trashes_list list = get_list_of_trashes(
                                                0   /// Additions
                                                );
    )", true);
}
