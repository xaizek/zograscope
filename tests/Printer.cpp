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

#include "Catch/catch.hpp"

#include <iostream>
#include <sstream>
#include <string>

#include "utils/strings.hpp"
#include "utils/time.hpp"
#include "Printer.hpp"
#include "compare.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Width of titles is considered on determining width", "[printer]")
{
    Tree oldTree = parseC("");
    Tree newTree = parseC("");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                    *oldTree.getLanguage(), oss);
    std::string expected;

    SECTION("e1/e2")
    {
        printer.addHeader({ "e1", "e2" });
        expected = normalizeText(R"(
            ~~~~~~~!~~~~~~~
             -  e1 !  +  e2
            ~~~~~~~!~~~~~~~
        )");
    }

    SECTION("left/right")
    {
        printer.addHeader({ "left", "right" });
        expected = normalizeText(R"(
            ~~~~~~~~~!~~~~~~~~~~
             -  left !  +  right
            ~~~~~~~~~!~~~~~~~~~~
        )");
    }

    printer.print(tr);

    REQUIRE(normalizeText(oss.str()) == expected);
}

TEST_CASE("Comment contents is compared", "[printer]")
{
    std::string printed = compareAndPrint(parseC("// This is that comment.\n"),
                                          parseC("// This is this comment.\n"),
                                          true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1  // This is {-that-} comment. {#~#}  1  // This is {+this+} comment.
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("String literal contents is compared", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        char str[] = "this is
        a
        string";
    )"), parseC(R"(
        char str[] = "this is
        the
        string";
    )"), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                |  1
         2          char str[] = "this is {#~#}  2          char str[] = "this is
         3          {-a-}                 {#~#}  3          {+the+}
         4          string";              {#~#}  4          string";
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Inner diffing does not mess up column tracking", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        format_str("...%s", str);
    )", true), parseC(R"(
        format_str("%s%s", ell, str);
    )", true));

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                |  1
         2  format_str("{-...-}%s", str); {#~#}  2  format_str("%s{+%s+}"{+,+}{+ +}{+ell+}, str);
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Comment contents is marked as moved on move", "[printer]")
{
    std::string printed = compareAndPrint(parseC(
R"(void f() {
    /* This is bad. */
}
    )", true), parseC(
R"(void f() {
    {
        /* Failure is bad. */
    }
}
    )", true), false);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1  void f() {                         |  1  void f() {
         -                                     {+++}  2      {+{+}
         2      {:/* :}{-This-}{: is bad. */:} {#~#}  3          {:/* :}{+Failure+}{: is bad. */:}
         -                                     {+++}  4      {+}+}
         3  }                                  |  5  }
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Lines with moves aren't folded", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        void f() {
        }

        void g()
        {
            movedStuff;
            movedStuff;
            movedStuff;
            movedStuff;
        }

        void h() {
        }
    )", true), parseC(R"(
        void h() {
        }

        void g()
        {
            movedStuff;
            movedStuff;
            movedStuff;
            movedStuff;
        }

        void f() {
        }
    )", true), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          1                                         |   1
          2  {:void:}{: :}{:f:}{:(:}{:):}{: :}{:{:} {---}   -
          3  {:}:}                                  {---}   -
          -                                         {+++}   2  void h() {
          -                                         {+++}   3  }
          4                                         |   4
          5  {:void:}{: :}{:g:}{:(:}{:):}           {#~#}   5  {:void:}{: :}{:g:}{:(:}{:):}
          6  {:{:}                                  {#~#}   6  {:{:}
          7      {:movedStuff:}{:;:}                {#~#}   7      {:movedStuff:}{:;:}
          8      {:movedStuff:}{:;:}                {#~#}   8      {:movedStuff:}{:;:}
          9      {:movedStuff:}{:;:}                {#~#}   9      {:movedStuff:}{:;:}
         10      {:movedStuff:}{:;:}                {#~#}  10      {:movedStuff:}{:;:}
         11  {:}:}                                  {#~#}  11  {:}:}
         12                                         |  12
         13  void h() {                             {---}  --
         14  }                                      {---}  --
         --                                         {+++}  13  {:void:}{: :}{:f:}{:(:}{:):}{: :}{:{:}
         --                                         {+++}  14  {:}:}
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Lines with additions/deletions aren't folded", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        int array[] = {
            somethingOldThatWontMatch,
            somethingOldThatWontMatch,
            somethingOldThatWontMatch,

            somethingCommon,
            somethingCommon,
            somethingCommon,
            somethingCommon,
            somethingCommon,
            somethingCommon,

            somethingOldThatWontMatch,
            somethingOldThatWontMatch,
            somethingOldThatWontMatch,
        };
    )", true), parseC(R"(
        int array[] = {
            aNewThingThatHasNothingInCommonWithTheOldOne,
            aNewThingThatHasNothingInCommonWithTheOldOne,
            aNewThingThatHasNothingInCommonWithTheOldOne,

            somethingCommon,
            somethingCommon,
            somethingCommon,
            somethingCommon,
            somethingCommon,
            somethingCommon,

            aNewThingThatHasNothingInCommonWithTheOldOne,
            aNewThingThatHasNothingInCommonWithTheOldOne,
            aNewThingThatHasNothingInCommonWithTheOldOne,
        };
    )", true), true);

    std::string expected = normalizeText(R"(
         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
           1                                         |   1
           2  int array[] = {-{-}                    {#~#}   2  int array[] = {+{+}
           3      {-somethingOldThatWontMatch-}{-,-} {---}   -
           4      {-somethingOldThatWontMatch-}{-,-} {---}   -
           5      {-somethingOldThatWontMatch-}{-,-} {---}   -
           -                                         {+++}   3      {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
           -                                         {+++}   4      {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
           -                                         {+++}   5      {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
           6                                         |   6
           7      {-somethingCommon-}{-,-}           {#~#}   7      {+somethingCommon+}{+,+}
           8      {-somethingCommon-}{-,-}           {#~#}   8      {+somethingCommon+}{+,+}
           9      {-somethingCommon-}{-,-}           {#~#}   9      {+somethingCommon+}{+,+}
          10      {-somethingCommon-}{-,-}           {#~#}  10      {+somethingCommon+}{+,+}
          11      {-somethingCommon-}{-,-}           {#~#}  11      {+somethingCommon+}{+,+}
          12      {-somethingCommon-}{-,-}           {#~#}  12      {+somethingCommon+}{+,+}
          13                                         |  13
          14      {-somethingOldThatWontMatch-}{-,-} {---}  --
          15      {-somethingOldThatWontMatch-}{-,-} {---}  --
          16      {-somethingOldThatWontMatch-}{-,-} {---}  --
          --                                         {+++}  14      {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
          --                                         {+++}  15      {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
          --                                         {+++}  16      {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
          17  {-}-};                                 {#~#}  17  {+}+};
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Highlighting skips leading whitespace", "[printer]")
{
    std::string printed = compareAndPrint(parseC(""), parseC(R"(
        /* This
         * is
         * a
         * comment */
    )"), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~
        {+++}  1
        {+++}  2          {+/* This+}
        {+++}  3           {+* is+}
        {+++}  4           {+* a+}
        {+++}  5           {+* comment */+}
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Highlighting fills background in a meaningful way", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        void func_proto(int a);
        int a;
        int b;
    )", true), parseC(R"(
        int a;
        int b;
        void func_prototype(int a);
    )", true), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                                              |  1
         2  {:void:}{: :}{#func_proto#}{:(:}{:int:}{: :}{:a:}{:):}{:;:} {---}  -
         3  int a;                                                      |  2  int a;
         4  int b;                                                      |  3  int b;
         -                                                              {+++}  4  {:void:}{: :}{#func_prototype#}{:(:}{:int:}{: :}{:a:}{:):}{:;:}
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Highlighting doesn't fill background where shouldn't 1", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        void f() {
            some_function(argument);
        }
    )", true), parseC(R"(
        void f() {
            some_function(argument + 1);
        }
    )", true), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                   |  1
         2  void f() {                       |  2  void f() {
         3      some_function({:argument:}); {#~#}  3      some_function({:argument:} {+++}{+ +}{+1+});
         4  }                                |  4  }
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Highlighting doesn't fill background where shouldn't 2", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        void f() {
            if (*p == 'T') {
                cfg.trunc_normal_sb_msgs = 1;
            } else if (*p == 'p') {
                cfg.shorten_title_paths = 1;
            }
        }
    )", true), parseC(R"(
        void f() {
            if (*p == 'M') {
                cfg.short_term_mux_titles = 1;
            } else if (*p == 'T') {
                cfg.trunc_normal_sb_msgs = 1;
            } else if (*p == 'p') {
                cfg.shorten_title_paths = 1;
            }
        }
    )", true), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                                                                              |   1
         2  void f() {                                                                                  |   2  void f() {
         -                                                                                              {+++}   3      {+if+}{+ +}{+(+}{+*+}{+p+}{+ +}{+==+}{+ +}{+'M'+}{+)+}{+ +}{+{+}
         -                                                                                              {+++}   4          {+cfg+}{+.+}{+short_term_mux_titles+}{+ +}{+=+}{+ +}{+1+}{+;+}
         3      {:if:}{: :}{:(:}{:*:}{:p:}{: :}{:==:}{: :}{:'T':}{:):}{: :}{:{:}                        {#~#}   5      {+}+}{+ +}{+else+} {:if:}{: :}{:(:}{:*:}{:p:}{: :}{:==:}{: :}{:'T':}{:):}{: :}{:{:}
         4          {:cfg:}{:.:}{:trunc_normal_sb_msgs:}{: :}{:=:}{: :}{:1:}{:;:}                       {#~#}   6          {:cfg:}{:.:}{:trunc_normal_sb_msgs:}{: :}{:=:}{: :}{:1:}{:;:}
         5      {:}:}{: :}{:else:}{: :}{:if:}{: :}{:(:}{:*:}{:p:}{: :}{:==:}{: :}{:'p':}{:):}{: :}{:{:} {#~#}   7      {:}:}{: :}{:else:}{: :}{:if:}{: :}{:(:}{:*:}{:p:}{: :}{:==:}{: :}{:'p':}{:):}{: :}{:{:}
         6          {:cfg:}{:.:}{:shorten_title_paths:}{: :}{:=:}{: :}{:1:}{:;:}                        {#~#}   8          {:cfg:}{:.:}{:shorten_title_paths:}{: :}{:=:}{: :}{:1:}{:;:}
         7      {:}:}                                                                                   {#~#}   9      {:}:}
         8  }                                                                                           |  10  }
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Widths is adjusted correctly on long headers", "[printer]")
{
    Tree oldTree = parseC("");
    Tree newTree = parseC("int a;");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                    *oldTree.getLanguage(), oss);

    std::string expected;

    oss.str({});
    printer.addHeader({ "left", "right" });
    printer.print(tr);
    expected = normalizeText(R"(
        ~~~~~~~~~~~~!~~~~~~~~~~~~~~~
         -  left    !  +  right
        ~~~~~~~~~~~~!~~~~~~~~~~~~~~~
        {+++}  1  {+int+}{+ +}{+a+}{+;+}
    )");
    CHECK(normalizeText(oss.str()) == expected);

    oss.str({});
    printer.addHeader({ "longleft", "right" });
    printer.print(tr);
    expected = normalizeText(R"(
        ~~~~~~~~~~~~~!~~~~~~~~~~~~~~
         -  left     !  +  right
         -  longleft !  +  right
        ~~~~~~~~~~~~~!~~~~~~~~~~~~~~
        {+++}  1  {+int+}{+ +}{+a+}{+;+}
    )");
    CHECK(normalizeText(oss.str()) == expected);

    oss.str({});
    printer.addHeader({ "left", "verylongright" });
    printer.print(tr);
    expected = normalizeText(R"(
        ~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~
         -  left     !  +  right
         -  longleft !  +  right
         -  left     !  +  verylongright
        ~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~
        {+++}  1  {+int+}{+ +}{+a+}{+;+}
    )");
    CHECK(normalizeText(oss.str()) == expected);
}

TEST_CASE("Deletions only leave only one side", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        /* This
         * is
         * a
         * comment */
    )"), parseC(""), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~
         1                            {---}
         2          {-/* This-}       {---}
         3           {-* is-}         {---}
         4           {-* a-}          {---}
         5           {-* comment */-} {---}
    )");

    CHECK(printed == expected);
}

TEST_CASE("Single side view doesn't contain blanks", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        struct Args {
            bool noRefine;
            bool gitDiff;
            bool gitRename;
            bool gitRenameOnly;
        };
    )"), parseC(R"(
        struct Args {
            bool noRefine;      // Don't run TED on updated nodes.
            bool gitDiff;       // Invoked by git and file was changed.
            bool gitRename;     // File was renamed and possibly changed too.
            bool gitRenameOnly; // File was renamed without changing it.
        };
    )"), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        |  1
        |  2  struct Args {
        {#~#}  3      bool noRefine;      {+// Don't run TED on updated nodes.+}
        {#~#}  4      bool gitDiff;       {+// Invoked by git and file was changed.+}
        {#~#}  5      bool gitRename;     {+// File was renamed and possibly changed too.+}
        {#~#}  6      bool gitRenameOnly; {+// File was renamed without changing it.+}
        |  7  };
    )");

    CHECK(printed == expected);
}

TEST_CASE("Adjacent updates aren't merged with background", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        void f() {
            if (someLongVariableName == 0) {
            }
        }
    )"), parseC(R"(
        void f() {
            if (someLongVariableName != 1) {
            }
        }
    )"), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                               |  1
         2  void f() {                                   |  2  void f() {
         3      if (someLongVariableName {#==#} {#0#}) { {#~#}  3      if (someLongVariableName {#!=#} {#1#}) {
         4      }                                        |  4      }
         5  }                                            |  5  }
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Separators in diffable tokens are handled separately", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        void f() {
            something("Destination doesn't exist");
        }
    )"), parseC(R"(
        void f() {
            something("Destination doesn't exist or not a directory");
        }
    )"), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                              |  1
         2  void f() {                                  |  2  void f() {
         3      something("Destination doesn't exist"); {#~#}  3      something("Destination doesn't exist{+ or not a directory+}");
         4  }                                           |  4  }
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Diffable identifiers are surrounded with brackets", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        void f() {
            cmd_group_open(undo_msg);
        }
    )"), parseC(R"(
        void f() {
            un_group_open(undo_msg);
        }
    )"), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                        |  1
         2  void f() {                            |  2  void f() {
         3      {-cmd-}{~_group_open~}(undo_msg); {#~#}  3      {+un+}{~_group_open~}(undo_msg);
         4  }                                     |  4  }
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Diffable identifiers that are too different aren't detailed",
          "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        void f() {
            cmd_group_begin(undo_msg);
        }
    )"), parseC(R"(
        void f() {
            un_group_open(undo_msg);
        }
    )"), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                     |  1
         2  void f() {                         |  2  void f() {
         3      {#cmd_group_begin#}(undo_msg); {#~#}  3      {#un_group_open#}(undo_msg);
         4  }                                  |  4  }
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Diffing by characters", "[printer]")
{
    std::string printed = compareAndPrint(parseC(R"(
        void f() {
            cmdGroupBegin(undo_msg);
        }
    )"), parseC(R"(
        void f() {
            unGroupOpen(undo_msg);
        }
    )"), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                                       |  1
         2  void f() {                                           |  2  void f() {
         3      {-cmd-}{~Group~}{-B-}{~e~}{-gi-}{~n~}(undo_msg); {#~#}  3      {+un+}{~Group~}{+Op+}{~en~}(undo_msg);
         4  }                                                    |  4  }
    )");

    REQUIRE(printed == expected);
}
