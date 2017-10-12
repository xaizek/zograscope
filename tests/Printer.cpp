#include "Catch/catch.hpp"

#include <iostream>

#include <boost/algorithm/string/trim.hpp>

#include "Printer.hpp"
#include "compare.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "utils.hpp"

#include "tests.hpp"

static std::string normalizeText(const std::string &s);

TEST_CASE("Multiline tokens don't mess up positioning", "[printer]")
{
    std::string input = R"(
        /* line1
         * line2 */  /* this */)";

    Tree tree = makeTree(input);

    std::string output = printSource(*tree.getRoot());
    CHECK(split(output, '\n') == split(input, '\n'));
}

TEST_CASE("Comment contents is compared", "[printer]")
{
    Tree oldTree = makeTree("// This is that comment.\n");
    Tree newTree = makeTree("// This is this comment.\n");

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, true);

    Printer printer(*oldTree.getRoot(), *newTree.getRoot());
    StreamCapture coutCapture(std::cout);
    printer.print(tr);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1  // This is {-that-} comment. ~  1  // This is {+this+} comment.
    )");

    REQUIRE(normalizeText(coutCapture.get()) == expected);
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

    Printer printer(*oldTree.getRoot(), *newTree.getRoot());
    StreamCapture coutCapture(std::cout);
    printer.print(tr);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                |  1
         2          char str[] = "this is |  2          char str[] = "this is
         3          {-a-}                 <  -
         -                                >  3          {+the+}
         4          string";              |  4          string";
    )");

    REQUIRE(normalizeText(coutCapture.get()) == expected);
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

    Printer printer(*oldTree.getRoot(), *newTree.getRoot());
    StreamCapture coutCapture(std::cout);
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

    REQUIRE(normalizeText(coutCapture.get()) == expected);
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
