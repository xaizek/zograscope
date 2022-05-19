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

#include <boost/algorithm/string/replace.hpp>

#include "c/C11SType.hpp"
#include "STree.hpp"
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

TEST_CASE("Coarse reduction links matched nodes", "[tree]")
{
    Tree oldTree = parseC(R"(
        int main(int argc, char *argv[]) {
            return 0;
        }
    )", true);

    Tree newTree = parseC(R"(
        int main(int argc, char *argv[]) {
            return 0;
        }
    )", true);

    reduceTreesCoarse(oldTree.getRoot(), newTree.getRoot());

    auto test = [&](const Node *node) {
        return node != oldTree.getRoot() && node != newTree.getRoot()
            && node->state == State::Unchanged && node->relative == nullptr;
    };
    CHECK(findNode(oldTree, test, true) == nullptr);
    CHECK(findNode(newTree, test, true) == nullptr);
}

TEST_CASE("Tabulation size in comments is variable", "[tree]")
{
    int tabWidth = 8;

    const char *const str = ""
        "/*\n"
        "\t * comment\n"
        "\t */"
    ;

    std::string expanded = str;
    boost::replace_all(expanded, "\t", std::string(8, ' '));

    cpp17::pmr::monolithic mr;
    std::unique_ptr<Language> lang = Language::create("file.c");

    TreeBuilder tb = lang->parse(str, "<input>", tabWidth, /*debug=*/false, mr);
    REQUIRE_FALSE(tb.hasFailed());

    STree stree(std::move(tb), str, false, false, *lang, mr);
    Tree tree(std::move(lang), tabWidth, str, stree.getRoot());

    const Node *node = findNode(tree,
                                [&](const Node *node) {
                                    if (node->type == Type::Comments) {
                                        if (!node->spelling.empty()) {
                                            return true;
                                        }
                                    }
                                    return false;
                                },
                                /*skipLastLayer=*/false);

    REQUIRE(node != nullptr);
    CHECK(node->spelling == expanded);
}
