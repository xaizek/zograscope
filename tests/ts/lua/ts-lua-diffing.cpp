// Copyright (C) 2021 xaizek <xaizek@posteo.net>
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

// These are tests of comparison and all of its phases.

#include "Catch/catch.hpp"

#include "tests.hpp"

TEST_CASE("Lua functions are matched", "[ts-lua][comparison]")
{
    diffTsLua(R"(
        function f(a)
            a.call()
            b.call()   --- Deletions
        end

        function g(a)
        end
    )", R"(
        function f(a)
            a.call()
        end

        function g(a)
            b.call()   --- Additions
        end
    )");
}

TEST_CASE("Variable is moved to table value", "[ts-lua][comparison]")
{
    diffTsLua(R"(
        function f()
            something()
            return
                text     --- Moves
        end
    )", R"(
        function f()
            something()
            return
            {            --- Additions
                text     --- Additions
                =        --- Additions
                text     --- Moves
            }            --- Additions
        end
    )");
}
