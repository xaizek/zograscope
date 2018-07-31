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

#include <fstream>
#include <memory>

#include "srcml/cxx/SrcmlCxxSType.hpp"
#include "utils/strings.hpp"
#include "TermHighlighter.hpp"
#include "TreeBuilder.hpp"
#include "tree.hpp"

#include "pmr/monolithic.hpp"

#include "tests.hpp"

using namespace srcmlcxx;

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
    std::string output = TermHighlighter(tree).print();
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
            switch (1) {
                default:break;
            }
        }
    )");
    CHECK(findNode(tree, makePred(Type::Keywords, "if")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Keywords, "return")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Keywords, "else")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Keywords, "switch")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Keywords, "default")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Keywords, "break")) != nullptr);
}

TEST_CASE("this is recognized as a keyword", "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        void Class::f() {
            this->a = 10;
        }
    )");
    CHECK(findNode(tree, makePred(Type::Keywords, "this")) != nullptr);
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

TEST_CASE("Directives are marked with types", "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        #include <something>
        #if 0
        #include "something"
        #elif defined(something)
        #endif // com
        #define macro(x) (x+x) /* that */ x
    )");
    CHECK(findNode(tree, makePred(Type::Directives, "#")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Directives, "include")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Directives, "if")) != nullptr);
    CHECK(findNode(tree, makePred(Type::IntConstants, "0")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Comments, "// com")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Comments, "/* that */")) != nullptr);
}

TEST_CASE("Identifiers are marked with types", "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        void Class::method() {
            int var;
        }
    )");
    CHECK(findNode(tree, makePred(Type::Identifiers, "Class")) != nullptr);
    CHECK(findNode(tree, makePred(Type::Identifiers, "var")) != nullptr);
}

TEST_CASE("Block nodes are spliced into their parents",
          "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        struct Struct {
            int callNesting = 0;
        };

        class Class {
            int callNesting = 0;
        };

        enum Enum {
            something
        };

        union Union {
            int a;
        };

        enum class EnumClass {
            something
        };

        SomeClass::SomeClass() {
            something;
        }

        SomeClass::~SomeClass() {
            something;
        }

        void f() {
            int a;
            if (a) {
                int b;
            } else {
                int c;
            }
            for (;;) {
                int d;
            }
            while (true) {
                int e;
            }
            do {
                int d;
            } while (true);
            switch (0) {
                case 1: break;
            }
        }
    )");

    auto test = [](const Node *node) {
        return (node->stype == +SrcmlCxxSType::Block);
    };
    CHECK(findNode(tree, test) == nullptr);
}

TEST_CASE("Constructors and destructors are moved to a separate layer",
          "[.srcml][srcml-cxx][parser]")
{
    Tree ctor = parseCxx(R"(
        SrcmlCxxLanguage::SrcmlCxxLanguage() {
            return;
        }
    )");
    Tree dtor = parseCxx(R"(
        SrcmlCxxLanguage::~SrcmlCxxLanguage() {
            return;
        }
    )");

    auto test = [](const Node *node) {
        return (node->stype == +SrcmlCxxSType::Constructor ||
                node->stype == +SrcmlCxxSType::Destructor)
            && node->children.empty()
            && node->next != nullptr;
    };
    CHECK(findNode(ctor, test) != nullptr);
    CHECK(findNode(dtor, test) != nullptr);
}

TEST_CASE("Braces of empty block are decomposed and stripped",
          "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        void f() {
        }
    )");
    std::string expected = "\nvoid f() {\n}";

    TermHighlighter hi(*tree.getRoot(), *tree.getLanguage());
    REQUIRE(hi.print() == expected);
}

TEST_CASE("Work around incorrect parsing of then/else block",
          "[.srcml][srcml-cxx][parser]")
{
    std::string input = normalizeText(R"(
        class Column {
            inline friend void f() {
                if( first )
                    first = false;
                else
                    os << "\n";
            }
        };
    )");
    Tree tree = parseCxx(input);

    TermHighlighter hi(*tree.getRoot(), *tree.getLanguage());
    REQUIRE(hi.print() + '\n' == input);
}

TEST_CASE("Unicode characters are handled correctly",
          "[.srcml][srcml-cxx][parser]")
{
    std::string input = R"(auto a = "µs";)";
    Tree tree = parseCxx(input);

    TermHighlighter hi(*tree.getRoot(), *tree.getLanguage());
    REQUIRE(hi.print() == input);
}

TEST_CASE("Working around srcml bug of handling stdin input",
          "[.srcml][srcml-cxx][srcml-bug][parser]")
{
    {
        TempFile tmpFile("zograscope-test");
        std::ofstream ofs(tmpFile);
        ofs << "C::C(){}";
        ofs.close();

        cpp17::pmr::monolithic mr;
        std::unique_ptr<Language> lang = Language::create("test-file.cpp");
        CHECK_FALSE(lang->parse("C::C(){}\n", tmpFile, false, mr).hasFailed());
    }

    {
        TempFile tmpFile("zograscope-test");
        std::ofstream ofs(tmpFile);
        ofs << "void\na::b()\n{\n}";
        ofs.close();

        cpp17::pmr::monolithic mr;
        std::unique_ptr<Language> lang = Language::create("test-file.cpp");
        CHECK_FALSE(lang->parse("void\na::b()\n{\n}", tmpFile, false,
                                mr).hasFailed());
    }
}

TEST_CASE("EOL continuation is identified in C++",
          "[.srcml][srcml-cxx][parser]")
{
    Tree tree = parseCxx(R"(
        int a \
          ;
    )");
    const Node *node = findNode(tree, [](const Node *node) {
                                    return node->label == "\\";
                                });
    REQUIRE(node != nullptr);
    CHECK(tree.getLanguage()->isEolContinuation(node));
}
