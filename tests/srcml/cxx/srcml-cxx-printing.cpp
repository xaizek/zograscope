// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include "utils/time.hpp"
#include "Printer.hpp"
#include "compare.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Comment contents is compared in C++", "[.srcml][srcml-cxx][printer]")
{
    Tree oldTree = parseCxx("// This is that comment.\n");
    Tree newTree = parseCxx("// This is this comment.\n");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, false);

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
