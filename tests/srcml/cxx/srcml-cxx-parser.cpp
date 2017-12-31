// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

// These are tests of basic properties of a parser.  Things like:
//  - whether some constructs can be parsed
//  - whether elements are identified correctly

#include "Catch/catch.hpp"

#include "utils/strings.hpp"
#include "Highlighter.hpp"
#include "tree.hpp"

#include "tests.hpp"

static const auto makePred = [](Type type, std::string label) {
    return [=](const Node *node) {
        return (node->type == type && node->label == label);
    };
};

TEST_CASE("Position of tokens is computed correctly",
          "[.srcml][srcml-cxx][parser]")
{
    // The problem here was common prefix of two adjacent tokens: `auto` and
    // `a`.
    std::string text = R"(auto a = "separator";)";

    Tree tree = parseCxx(text);
    std::string output = Highlighter(tree).print();
    CHECK(output == text);
}

TEST_CASE("Literals are marked with types", "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        auto a = true || false;
        auto b = 'a' + L'a';
        auto c = "a" L"a";
        auto d = nullptr;
        auto e = 10 + 0x1 + 001;
        auto f = 10i+1;
    )");
    CHECK(findNode(tree, makePred(Type::IntConstants, "true")) != nullptr);
    CHECK(findNode(tree, makePred(Type::IntConstants, "false")) != nullptr);
    CHECK(findNode(tree, makePred(Type::CharConstants, "'a'")) != nullptr);
    CHECK(findNode(tree, makePred(Type::StrConstants, R"("a")")) != nullptr);
    CHECK(findNode(tree, makePred(Type::StrConstants, R"(L"a")")) != nullptr);
    CHECK(findNode(tree, makePred(Type::IntConstants, "nullptr")) != nullptr);
    CHECK(findNode(tree, makePred(Type::IntConstants, "10")) != nullptr);
    CHECK(findNode(tree, makePred(Type::IntConstants, "0x1")) != nullptr);
    CHECK(findNode(tree, makePred(Type::IntConstants, "001")) != nullptr);
    CHECK(findNode(tree, makePred(Type::FPConstants, "10i+1")) != nullptr);
}

TEST_CASE("Operators are marked with types", "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        void f() {
            a += 1;
        }
    )");
    CHECK(findNode(tree, makePred(Type::Operators, "+=")) != nullptr);
}

TEST_CASE("Types are marked with types", "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        void f() {
            User a;
        }
    )");
    CHECK(findNode(tree, makePred(Type::Keywords, "void")) != nullptr);
    CHECK(findNode(tree, makePred(Type::UserTypes, "User")) != nullptr);
}

TEST_CASE("Specifiers are marked with types", "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx("const int a;");
    CHECK(findNode(tree, makePred(Type::Specifiers, "const")) != nullptr);
}

TEST_CASE("Brackets are marked with types", "[.srcml][srcml-cxx][parser]")
{
    Tree tree;

    tree = parseCxx(R"(
        void f(int arg) {
            int a[arg];
        }
    )");
    CHECK(findNode(tree, makePred(Type::LeftBrackets, "(")) != nullptr);
    CHECK(findNode(tree, makePred(Type::LeftBrackets, "{")) != nullptr);
    CHECK(findNode(tree, makePred(Type::LeftBrackets, "[")) != nullptr);
    CHECK(findNode(tree, makePred(Type::RightBrackets, ")")) != nullptr);
    CHECK(findNode(tree, makePred(Type::RightBrackets, "}")) != nullptr);
    CHECK(findNode(tree, makePred(Type::RightBrackets, "]")) != nullptr);

    tree = parseCxx(R"(
        int a = (1 + 2);
    )");
    CHECK(findNode(tree, makePred(Type::LeftBrackets, "(")) != nullptr);
    CHECK(findNode(tree, makePred(Type::RightBrackets, ")")) != nullptr);
}

TEST_CASE("Keywords are marked with types", "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        void f() {
            if (0) {
                return  ;
            } else {
            }
        }
    )");
    CHECK(findNode(tree, makePred(Type::Keywords, "if")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Keywords, "return")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Keywords, "else")) != nullptr);
}

TEST_CASE("Function names are marked with types", "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        void func() {
            call();
            obj.as<int>();
        }
        void Class::method() {
            obj.meth();
        }
    )");
    CHECK(findNode(tree, makePred(Type::Functions, "func")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Functions, "call")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Functions, "as")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Functions, "Class")) == nullptr);
    CHECK(findNode(tree, makePred(Type::Functions, "method")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Functions, "meth")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Functions, "obj")) == nullptr);
}

TEST_CASE("Comments are marked with types", "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        /* mlcom */
        // slcom
    )");
    CHECK(findNode(tree, makePred(Type::Comments, "/* mlcom */")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Comments, "// slcom")) != nullptr);
}
