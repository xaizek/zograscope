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

#include "tooling/Matcher.hpp"
#include "mtypes.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Comment matcher works", "[tooling][matcher][.srcml]")
{
    Matcher matcher(MType::Comment, nullptr);

    int nMatches = 0;

    auto matchHandler = [&](Node */*node*/) {
        ++nMatches;
    };

    SECTION("In Make") {
        Tree tree = parseMake("# this is # a comment");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 1);
    }
    SECTION("In C") {
        Tree tree = parseC("/* a */ /* b // b */ // c", true);
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 3);
    }
    SECTION("In C++") {
        Tree tree = parseCxx("/* a */ /* b // b */ // c");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 3);
    }
    SECTION("In Lua") {
        Tree tree = parseLua("--[[a]] --[[b]]");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 2);
    }
}

TEST_CASE("Directive matcher works", "[tooling][matcher][.srcml]")
{
    Matcher matcher(MType::Directive, nullptr);

    int nMatches = 0;

    auto matchHandler = [&](Node */*node*/) {
        ++nMatches;
    };

    SECTION("In Make") {
        Tree tree = parseMake("-include config.mk");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 1);
    }
    SECTION("In C") {
        Tree tree = parseC("#include <stdio.h>\n#define a \\b", true);
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 2);
    }
    SECTION("In C++") {
        Tree tree = parseCxx("#include <iostream>\n#define a \\b");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 2);
    }
}

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
    SECTION("In Lua") {
        Tree tree = parseLua("function f() a() v = b() c() end");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 3);
    }
}

TEST_CASE("Block matcher works", "[tooling][matcher][.srcml]")
{
    Matcher matcher(MType::Block, nullptr);

    int nMatches = 0;

    auto matchHandler = [&](Node */*node*/) {
        ++nMatches;
    };

    SECTION("In C") {
        Tree tree = parseC("void f() { { } { { } } }", true);
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 4);
    }
    SECTION("In C++") {
        Tree tree = parseCxx("void f() { { } { { } } }");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 4);
    }
    SECTION("In Lua") {
        Tree tree = parseLua("function f() do end end");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 2);
    }
}

TEST_CASE("Call matcher works", "[tooling][matcher][.srcml]")
{
    Matcher matcher(MType::Call, nullptr);

    int nMatches = 0;

    auto matchHandler = [&](Node */*node*/) {
        ++nMatches;
    };

    SECTION("In Make") {
        Tree tree = parseMake("$(info $(addprefix bla,bla))");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 2);
    }
    SECTION("In C") {
        Tree tree = parseC("void f() { call1(nested()); call2(); }", true);
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 3);
    }
    SECTION("In C++") {
        Tree tree = parseCxx("void f() { call1(); call2(nested()); }");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 3);
    }
    SECTION("In Lua") {
        Tree tree = parseLua("call1() call2()");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 2);
    }
}

TEST_CASE("Parameter matcher works", "[tooling][matcher][.srcml]")
{
    Matcher matcher(MType::Parameter, nullptr);

    int nMatches = 0;

    auto matchHandler = [&](Node */*node*/) {
        ++nMatches;
    };

    SECTION("In C") {
        Tree tree = parseC("void f(int a1, int a2, int a3, int a4) { }", true);
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 4);
    }
    SECTION("In C++") {
        Tree tree = parseCxx("void f(int a1, int a2, int a3, int a4) { }");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 4);
    }
    SECTION("In Lua") {
        Tree tree = parseLua("function f(a1, a2, a3, a4) ; end");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 4);
    }
}

TEST_CASE("Declaration matcher works", "[tooling][matcher][.srcml]")
{
    Matcher matcher(MType::Declaration, nullptr);

    int nMatches = 0;

    auto matchHandler = [&](Node */*node*/) {
        ++nMatches;
    };

    SECTION("In C") {
        Tree tree = parseC("int a1; int a2; int a3; int a4;", true);
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 4);
    }
    SECTION("In C++") {
        Tree tree = parseCxx("int a1; int a2; int a3; int a4;");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 4);
    }
    SECTION("In Lua") {
        Tree tree = parseLua("local a1; local a2; local a3; local a4");
        CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
        CHECK(nMatches == 4);
    }
}
