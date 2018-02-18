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

#include "c/C11SType.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Locations are propagated to inner (non-leaf) nodes", "[tree]")
{
    using namespace c11stypes;

    Tree tree = parseC(R"(
            int main(int argc, char *argv[]) {
                return 0;
            }
    )", true);

    auto test = [](const Node *node) {
        return (node->stype == +C11SType::ReturnValueStmt);
    };
    const Node *const node = findNode(tree, test);
    REQUIRE(node != nullptr);

    CHECK(node->line == 3);
    CHECK(node->col == 17);
}
