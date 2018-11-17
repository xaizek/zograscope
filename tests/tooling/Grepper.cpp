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

#include "tooling/Grepper.hpp"
#include "tooling/Matcher.hpp"
#include "mtypes.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Empty grepper matches nothing but succeeds", "[tooling][grepper]")
{
    auto grepHandler = [&](const std::vector<Node *> &/*match*/) { };

    Tree tree = parseC("void chars(const char *, const char);", true);

    Grepper grepper;
    CHECK(grepper.grep(tree.getRoot(), grepHandler));
    CHECK(grepper.getSeen() == 0);
    CHECK(grepper.getMatched() == 0);
}

TEST_CASE("Grep expressions", "[tooling][grepper]")
{
    auto grepHandler = [&](const std::vector<Node *> &/*match*/) { };

    Tree tree = parseC("void chars(const char *, const char);", true);

    SECTION("Wildcard") {
        Grepper grepper({ "const", "//" });
        CHECK(grepper.grep(tree.getRoot(), grepHandler));
        CHECK(grepper.getMatched() == 2);
    }
    SECTION("Prefix") {
        Grepper grepper({ "/^vo/" });
        CHECK(grepper.grep(tree.getRoot(), grepHandler));
        CHECK(grepper.getMatched() == 1);
    }
    SECTION("Suffix") {
        Grepper grepper({ "/r$/" });
        CHECK(grepper.grep(tree.getRoot(), grepHandler));
        CHECK(grepper.getMatched() == 2);
    }
    SECTION("Substring") {
        Grepper grepper({ "/har/" });
        CHECK(grepper.grep(tree.getRoot(), grepHandler));
        CHECK(grepper.getMatched() == 3);
    }
    SECTION("Exact") {
        Grepper grepper({ "/^char$/" });
        CHECK(grepper.grep(tree.getRoot(), grepHandler));
        CHECK(grepper.getMatched() == 2);
    }
    SECTION("Regexp") {
        Grepper grepper({ "//[(*,);]/" });
        CHECK(grepper.grep(tree.getRoot(), grepHandler));
        CHECK(grepper.getMatched() == 5);
    }
}

TEST_CASE("Grep starts each processing with clear match", "[tooling][grepper]")
{
    Matcher matcher(MType::Parameter, nullptr);
    Grepper grepper({ "*", "const" });

    int nMatches = 0, nGreps = 0;

    auto matchHandler = [&](Node *node) {
        auto grepHandler = [&](const std::vector<Node *> &/*match*/) {
            ++nGreps;
        };

        grepper.grep(node, grepHandler);
        ++nMatches;
    };

    Tree tree = parseC("void f(const char *, const char);", true);

    CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
    CHECK(nMatches == 2);
    CHECK(nGreps == 0);
}
