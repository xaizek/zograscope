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

#include "tooling/Matcher.hpp"
#include "mtypes.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Statement matcher works", "[tooling][matcher][.srcml]")
{
    Matcher matcher(MType::Statement, nullptr);

    int nMatches = 0;

    auto matchHandler = [&](Node */*node*/) {
        ++nMatches;
    };

    SECTION("In C") {
        Tree tree = parseC("void f() { stmt1(); stmt2(); stmt3(); }", true);
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 3);
    }
    SECTION("In C++") {
        Tree tree = parseCxx("void f() { stmt1(); stmt2(); stmt3(); }");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 3);
    }
}