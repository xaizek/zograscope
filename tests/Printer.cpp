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

#include "Catch/catch.hpp"

#include <iostream>
#include <sstream>
#include <string>

#include <boost/algorithm/string/trim.hpp>

#include "utils/strings.hpp"
#include "utils/time.hpp"
#include "Printer.hpp"
#include "compare.hpp"
#include "tree.hpp"

#include "tests.hpp"

static std::string normalizeText(const std::string &s);

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
    Tree oldTree = parseC("// This is that comment.\n");
    Tree newTree = parseC("// This is this comment.\n");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                    *oldTree.getLanguage(), oss);
    printer.print(tr);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1  // This is {-that-} comment. ~  1  // This is {+this+} comment.
    )");

    REQUIRE(normalizeText(oss.str()) == expected);
}

TEST_CASE("String literal contents is compared", "[printer]")
{
    Tree oldTree = parseC(R"(
        char str[] = "this is
        a
        string";
    )");
    Tree newTree = parseC(R"(
        char str[] = "this is
        the
        string";
    )");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                    *oldTree.getLanguage(), oss);
    printer.print(tr);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                |  1
         2          char str[] = "this is ~  2          char str[] = "this is
         3          {-a-}                 <  -
         -                                >  3          {+the+}
         4          string";              ~  4          string";
    )");

    REQUIRE(normalizeText(oss.str()) == expected);
}

TEST_CASE("Inner diffing does not mess up column tracking", "[printer]")
{
    Tree oldTree = parseC(R"(
        format_str("...%s", str);
    )", true);
    Tree newTree = parseC(R"(
        format_str("%s%s", ell, str);
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, false);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                    *oldTree.getLanguage(), oss);
    printer.print(tr);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                        |  1
         2          format_str({-"...%s"-}, str); ~  2          format_str({+"%s%s"+}{+,+} {+ell+}, str);
    )");

    REQUIRE(normalizeText(oss.str()) == expected);
}

TEST_CASE("Comment contents is not marked as updated on move", "[printer]")
{
    Tree oldTree = parseC(
R"(void f() {
    /* This is bad. */
}
    )");
    Tree newTree = parseC(
R"(void f() {
    {
        /* Failure is bad. */
    }
}
    )");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                    *oldTree.getLanguage(), oss);
    printer.print(tr);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1  void f() {                 |  1  void f() {
         -                             >  2      {+{+}
         -                             >  3          /* {+Failure+} is bad. */
         -                             >  4      {+}+}
         2      /* {-This-} is bad. */ <  -
         3  }                          |  5  }
    )");

    REQUIRE(normalizeText(oss.str()) == expected);
}

TEST_CASE("Lines with moves aren't folded", "[printer]")
{
    Tree oldTree = parseC(R"(
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
    )", true);
    Tree newTree = parseC(R"(
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
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                    *oldTree.getLanguage(), oss);
    printer.print(tr);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          1                                         |   1
          2          {:void:} {:f:}{:(:}{:):} {:{:} ~   2          void h() {
          3          {:}:}                          ~   3          }
          4                                         |   4
          5          {:void:} {:g:}{:(:}{:):}       ~   5          {:void:} {:g:}{:(:}{:):}
          6          {:{:}                          ~   6          {:{:}
          7              {:movedStuff:}{:;:}        ~   7              {:movedStuff:}{:;:}
          8              {:movedStuff:}{:;:}        ~   8              {:movedStuff:}{:;:}
          9              {:movedStuff:}{:;:}        ~   9              {:movedStuff:}{:;:}
         10              {:movedStuff:}{:;:}        ~  10              {:movedStuff:}{:;:}
         11          {:}:}                          ~  11          {:}:}
         12                                         |  12
         13          void h() {                     ~  13          {:void:} {:f:}{:(:}{:):} {:{:}
         14          }                              ~  14          {:}:}
    )");

    REQUIRE(normalizeText(oss.str()) == expected);
}

TEST_CASE("Lines with additions/deletions aren't folded", "[printer]")
{
    Tree oldTree = parseC(R"(
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
    )", true);
    Tree newTree = parseC(R"(
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
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                    *oldTree.getLanguage(), oss);
    printer.print(tr);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          1                                                 |   1
          2          int array[] = {-{-}                    ~   2          int array[] = {+{+}
          3              {-somethingOldThatWontMatch-}{-,-} <   -
          4              {-somethingOldThatWontMatch-}{-,-} <   -
          5              {-somethingOldThatWontMatch-}{-,-} <   -
          -                                                 >   3              {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
          -                                                 >   4              {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
          -                                                 >   5              {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
          6                                                 |   6
          7              {-somethingCommon-}{-,-}           ~   7              {+somethingCommon+}{+,+}
          8              {-somethingCommon-}{-,-}           ~   8              {+somethingCommon+}{+,+}
          9              {-somethingCommon-}{-,-}           ~   9              {+somethingCommon+}{+,+}
          10              {-somethingCommon-}{-,-}           ~  10              {+somethingCommon+}{+,+}
          11              {-somethingCommon-}{-,-}           ~  11              {+somethingCommon+}{+,+}
          12              {-somethingCommon-}{-,-}           ~  12              {+somethingCommon+}{+,+}
          13                                                 |  13
          14              {-somethingOldThatWontMatch-}{-,-} <  --
          15              {-somethingOldThatWontMatch-}{-,-} <  --
          16              {-somethingOldThatWontMatch-}{-,-} <  --
          --                                                 >  14              {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
          --                                                 >  15              {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
          --                                                 >  16              {+aNewThingThatHasNothingInCommonWithTheOldOne+}{+,+}
          17          {-}-};                                 ~  17          {+}+};
    )");

    REQUIRE(normalizeText(oss.str()) == expected);
}

TEST_CASE("Highlighting skips leading whitespace", "[printer]")
{
    Tree oldTree = parseC("");
    Tree newTree = parseC(R"(
        /* This
         * is
         * a
         * comment */
    )");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                    *oldTree.getLanguage(), oss);
    printer.print(tr);

    std::string expected = normalizeText(R"(
        ~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         -   >  1
         -   >  2          {+/* This+}
         -   >  3           {+* is+}
         -   >  4           {+* a+}
         -   >  5           {+* comment */+}
    )");

    REQUIRE(normalizeText(oss.str()) == expected);
}

static std::string
normalizeText(const std::string &s)
{
    std::string result;
    for (boost::string_ref sr : split(s, '\n')) {
        std::string line = sr.to_string();
        boost::trim_if(line, boost::is_any_of("\r\n \t"));

        if (!line.empty()) {
            result += line;
            result += '\n';
        }
    }
    return result;
}
