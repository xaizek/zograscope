// Copyright (C) 2019 xaizek <xaizek@posteo.net>
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

#include "tooling/FunctionAnalyzer.hpp"
#include "mtypes.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Function with no args on one line", "[tooling][function-analyzer]")
{
    Tree tree = parseC("void f() { }", true);

    auto test = [&](const Node *node) {
        return (tree.getLanguage()->classify(node->stype) == MType::Function);
    };
    const Node *func = findNode(tree, test, true);
    REQUIRE(func != nullptr);

    FunctionAnalyzer functionAnalyzer(*tree.getLanguage());
    CHECK(functionAnalyzer.getLineCount(func) == 1);
    CHECK(functionAnalyzer.getParamCount(func) == 0);
}

TEST_CASE("Function with args on many lines", "[tooling][function-analyzer]")
{
    Tree tree = parseC(R"(
        void
        f
        (int a, int b)
        {

        }
    )", true);

    auto test = [&](const Node *node) {
        return (tree.getLanguage()->classify(node->stype) == MType::Function);
    };
    const Node *func = findNode(tree, test, true);
    REQUIRE(func != nullptr);

    FunctionAnalyzer functionAnalyzer(*tree.getLanguage());
    CHECK(functionAnalyzer.getLineCount(func) == 6);
    CHECK(functionAnalyzer.getParamCount(func) == 2);
}

TEST_CASE("C function with void args", "[tooling][function-analyzer]")
{
    Tree tree = parseC("void f(void) { }", true);

    auto test = [&](const Node *node) {
        return (tree.getLanguage()->classify(node->stype) == MType::Function);
    };
    const Node *func = findNode(tree, test, true);
    REQUIRE(func != nullptr);

    FunctionAnalyzer functionAnalyzer(*tree.getLanguage());
    CHECK(functionAnalyzer.getLineCount(func) == 1);
    CHECK(functionAnalyzer.getParamCount(func) == 0);
}

TEST_CASE("C++ function with void args", "[.srcml][tooling][function-analyzer]")
{
    Tree tree = parseCxx("void f(void) { }");

    auto test = [&](const Node *node) {
        return (tree.getLanguage()->classify(node->stype) == MType::Function);
    };
    const Node *func = findNode(tree, test, true);
    REQUIRE(func != nullptr);

    FunctionAnalyzer functionAnalyzer(*tree.getLanguage());
    CHECK(functionAnalyzer.getLineCount(func) == 1);
    CHECK(functionAnalyzer.getParamCount(func) == 0);
}

TEST_CASE("C function with extern prototype", "[tooling][function-analyzer]")
{
    Tree tree = parseC(R"(
        void f(int a) {
            extern void anotherOne(float something);
        }
    )", true);

    auto test = [&](const Node *node) {
        return (tree.getLanguage()->classify(node->stype) == MType::Function);
    };
    const Node *func = findNode(tree, test, true);
    REQUIRE(func != nullptr);

    FunctionAnalyzer functionAnalyzer(*tree.getLanguage());
    CHECK(functionAnalyzer.getLineCount(func) == 3);
    CHECK(functionAnalyzer.getParamCount(func) == 1);
}

TEST_CASE("C++ function with a lambda", "[.srcml][tooling][function-analyzer]")
{
    Tree tree = parseCxx(R"(
        void f(int a) {
            auto func = [](int b, double c) {
            };
        }
    )");

    auto test = [&](const Node *node) {
        return (tree.getLanguage()->classify(node->stype) == MType::Function);
    };
    const Node *func = findNode(tree, test, true);
    REQUIRE(func != nullptr);

    FunctionAnalyzer functionAnalyzer(*tree.getLanguage());
    CHECK(functionAnalyzer.getLineCount(func) == 4);
    CHECK(functionAnalyzer.getParamCount(func) == 1);
}
