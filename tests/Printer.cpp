#include "Catch/catch.hpp"

#include <iostream>

#include "Printer.hpp"
#include "compare.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "utils.hpp"

#include "tests.hpp"

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

    std::string expected =
R"(~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 1  // This is {-that-} comment. <  -                              
 -                               >  1  // This is {+this+} comment.
)";

    REQUIRE(coutCapture.get() == expected);
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

    std::string expected =
R"(~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 1                                |  1  
 2          char str[] = "this is |  2          char str[] = "this is
 3          {-a-}                 <  -                               
 -                                >  3          {+the+}
 4          string";              |  4          string";
)";

    REQUIRE(coutCapture.get() == expected);
}
