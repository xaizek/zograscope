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

#include <string>

#include "utils/strings.hpp"
#include "Highlighter.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Multiline tokens don't mess up positioning", "[highlighter]")
{
    std::string input = R"(
        /* line1
         * line2 */  /* this */)";

    Tree tree = parseC(input);

    std::string output = Highlighter(*tree.getRoot()).print();
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

    Highlighter hi(*tree.getRoot());
    CHECK(hi.print(1, 2) == "/* line1\n");
    CHECK(hi.print(4, 2) == "// line4\n/* line5");
    CHECK(hi.print(7, 1) == " * line7 */");
    CHECK(hi.print(7, 1) == "");
    CHECK(hi.print(6, 3) == "// line8");
    CHECK(hi.print(10, 1) == " * line10");
    CHECK(hi.print(14, 10) == "// line14");
    CHECK(hi.print(20, 10) == "");
}
