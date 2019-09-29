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

#include "utils/time.hpp"
#include "compare.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Distilling links matched nodes", "[change-distiller]")
{
    Tree oldTree = parseC(R"(
        int main(int argc, char *argv[]) {
            return 0;
        }
    )", true);

    Tree newTree = parseC(R"(
        int main(int argc, char *argv[]) {
            return 1;
        }
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, false);

    auto test = [&](const Node *node) {
        return node != oldTree.getRoot() && node != newTree.getRoot()
            && node->state == State::Unchanged && node->relative == nullptr;
    };
    CHECK(findNode(oldTree, test, true) == nullptr);
    CHECK(findNode(newTree, test, true) == nullptr);
}
