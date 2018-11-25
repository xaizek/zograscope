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

// These are tests that check things which aren't visible without printing.

#include "Catch/catch.hpp"

#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Directives are diffed", "[printer]")
{
    std::string printed = compareAndPrint(
        parseC("#define DEFINE (1 + 2 + 3)", true),
        parseC("#define DEFINE (1 + 2 + 3 + 4)", true)
    );

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1  #define DEFINE (1 + 2 + 3) ~  1  #define DEFINE (1 + 2 + 3 {+++} {+4+})
    )");

    REQUIRE(printed == expected);
}
