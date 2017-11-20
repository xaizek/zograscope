#include "Catch/catch.hpp"

#include <iostream>
#include <sstream>
#include <string>

#include <boost/algorithm/string/trim.hpp>

#include "utils/strings.hpp"
#include "Printer.hpp"
#include "compare.hpp"
#include "time.hpp"
#include "tree.hpp"

#include "tests.hpp"

static std::string normalizeText(const std::string &s);

TEST_CASE("Width of titles is considered on determining width", "[printer]")
{
    Tree oldTree = makeTree("");
    Tree newTree = makeTree("");

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(), oss);
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
    Tree oldTree = makeTree("// This is that comment.\n");
    Tree newTree = makeTree("// This is this comment.\n");

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(), oss);
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
    Tree oldTree = makeTree(R"(
        char str[] = "this is
        a
        string";
    )");
    Tree newTree = makeTree(R"(
        char str[] = "this is
        the
        string";
    )");

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(), oss);
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

TEST_CASE("Comment contents is not marked as updated on move", "[printer]")
{
    Tree oldTree = makeTree(
R"(void f() {
    /* This is bad. */
}
    )");
    Tree newTree = makeTree(
R"(void f() {
    {
        /* Failure is bad. */
    }
}
    )");

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(), oss);
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

TEST_CASE("Lines with changes aren't folded", "[printer]")
{
    Tree oldTree = makeTree(R"(
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
    Tree newTree = makeTree(R"(
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
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(), oss);
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
