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

    Tree tree = makeTree(input);

    std::string output = Highlighter().print(*tree.getRoot());
    CHECK(split(output, '\n') == split(input, '\n'));
}
