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

#include <string>

#include "utils/strings.hpp"
#include "utils/time.hpp"
#include "TermHighlighter.hpp"
#include "compare.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Multiline tokens don't mess up positioning", "[highlighter]")
{
    std::string input = R"(
        /* line1
         * line2 */  /* this */)";

    Tree tree = parseC(input);

    std::string output = TermHighlighter(tree).print();
    CHECK(split(output, '\n') == split(input, '\n'));
}

TEST_CASE("Ranges are printed correctly", "[highlighter]")
{
    Tree tree = parseC(
R"(/* line1

 * line3 */
// line4
/* line5
 * line6
 * line7 */
// line8
/* line9
 * line10
 * line11
 * line12 */
int f() { return 10; }
// line14)", true);

    TermHighlighter hi(tree);
    CHECK(hi.print(1, 2) == "/* line1\n");
    CHECK(hi.print(4, 2) == "// line4\n/* line5");
    CHECK(hi.print(7, 1) == " * line7 */");
    CHECK(hi.print(7, 1) == "");
    CHECK(hi.print(6, 3) == "// line8");
    CHECK(hi.print(10, 1) == " * line10");
    CHECK(hi.print(14, 10) == "// line14");
    CHECK(hi.print(20, 10) == "");
}

TEST_CASE("Printing a subtree", "[highlighter]")
{
    Tree tree = parseC(R"(
        // line1
        // line2
    )", true);

    const Node *const node = findNode(tree, Type::Comments, "// line2");
    REQUIRE(node != nullptr);

    TermHighlighter hi(*node, *tree.getLanguage(), true, node->line);
    CHECK(hi.print() == "// line2");
}

TEST_CASE("References are printed", "[highlighter]")
{
    Tree oldTree = parseC("int oldVarName;");
    Tree newTree = parseC("int newVarName;");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    TermHighlighter oldHi(oldTree, true);
    oldHi.setPrintReferences(true);
    CHECK(oldHi.print() == "int {-old-}{~VarName~}{1};");

    TermHighlighter newHi(newTree, false);
    newHi.setPrintReferences(true);
    CHECK(newHi.print() == "int {+new+}{~VarName~}{1};");
}

TEST_CASE("Brackets can be disabled", "[highlighter]")
{
    Tree oldTree = parseC("int oldVarName;");
    Tree newTree = parseC("int newVarName;");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    TermHighlighter oldHi(oldTree, true);
    oldHi.setPrintBrackets(false);
    CHECK(oldHi.print() == "int {-old-}{~VarName~};");

    TermHighlighter newHi(newTree, false);
    newHi.setPrintBrackets(false);
    CHECK(newHi.print() == "int {+new+}{~VarName~};");
}

TEST_CASE("Diffables can be non-transparent", "[highlighter]")
{
    Tree oldTree = parseC("int oldVarName;");
    Tree newTree = parseC("int newVarName;");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    TermHighlighter oldHi(oldTree, true);
    oldHi.setTransparentDiffables(false);
    CHECK(oldHi.print() == "int {-old-}{~VarName~};");

    TermHighlighter newHi(newTree, false);
    newHi.setTransparentDiffables(false);
    CHECK(newHi.print() == "int {+new+}{~VarName~};");
}

TEST_CASE("Non-transparent diffables have background filled", "[highlighter]")
{
    Tree oldTree = parseC("// aa bb cc");
    Tree newTree = parseC("// aa bb dd");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    TermHighlighter oldHi(oldTree, true);
    oldHi.setTransparentDiffables(false);
    CHECK(oldHi.print() == "// aa bb {-cc-}");

    TermHighlighter newHi(newTree, false);
    newHi.setTransparentDiffables(false);
    CHECK(newHi.print() == "// aa bb {+dd+}");
}
