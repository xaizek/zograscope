// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Content of string literals is compared in Make", "[make][printer]")
{
    std::string printed = compareAndPrint(
        parseMake("a = '#define VERSION 0.9'"),
        parseMake("a = '#define VERSION 0.10'")
    );

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1  a = '#define VERSION 0.{-9-}' {#~#}  1  a = '#define VERSION 0.{+10+}'
    )");

    REQUIRE(printed == expected);
}
