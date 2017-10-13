#include "Catch/catch.hpp"

#include <string>

#include "Highlighter.hpp"
#include "tree.hpp"
#include "utils.hpp"

#include "tests.hpp"

TEST_CASE("Multiline tokens don't mess up positioning", "[highlighter]")
{
    std::string input = R"(
        /* line1
         * line2 */  /* this */)";

    Tree tree = makeTree(input);

    std::string output = Highlighter().print(*tree.getRoot());
    CHECK(split(output, '\n') == split(input, '\n'));
}
