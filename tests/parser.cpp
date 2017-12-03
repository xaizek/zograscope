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

#include <boost/algorithm/string/predicate.hpp>

#include <functional>
#include <iostream>

#include "TreeBuilder.hpp"
#include "stypes.hpp"
#include "tree.hpp"
#include "types.hpp"

#include "tests.hpp"

static int countNodes(const Node &root);

TEST_CASE("Empty input is OK", "[parser][extensions]")
{
    CHECK(cIsParsed(""));
    CHECK(cIsParsed("      "));
    CHECK(cIsParsed("\t\n \t \n"));

    Tree tree = makeTree("");
    CHECK(tree.getRoot()->stype == SType::TranslationUnit);
}

TEST_CASE("Missing final newline is added", "[parser][extensions]")
{
    Tree tree;

    tree = makeTree("\n// Comment");
    CHECK(findNode(tree, Type::Comments, "// Comment") != nullptr);

    // Parsing such line second time messed up state of the lexer and caused
    // attempt to allocate (size_t)-1 bytes.
    tree = makeTree("\n// Comment");
    CHECK(findNode(tree, Type::Comments, "// Comment") != nullptr);
}

TEST_CASE("Non-UNIX EOLs are allowed", "[parser]")
{
    CHECK(cIsParsed("int\r\na\r;\n"));
}

TEST_CASE("Error message counts tabulation as single character", "[parser]")
{
    StreamCapture coutCapture(std::cout);
    CHECK_FALSE(cIsParsed("\t\x01;"));
    REQUIRE(boost::starts_with(coutCapture.get(), "<input>:1:2:"));
}

TEST_CASE("Top-level macros are parsed successfully", "[parser][extensions]")
{
    CHECK(cIsParsed("TSTATIC_DEFS(int func(type *var);)"));
    CHECK(cIsParsed("ARRAY_GUARD(sort_enum, 1 + SK_COUNT);"));
    CHECK(cIsParsed("XX(state_t) st;"));
    CHECK(cIsParsed("MACRO();"));
}

TEST_CASE("Empty initializer list is parsed", "[parser][extensions]")
{
    CHECK(cIsParsed("args_t args = {};"));
}

TEST_CASE("Bit field is parsed", "[parser][conflicts]")
{
    CHECK(cIsParsed("struct s { unsigned int stuff : 1; };"));
}

TEST_CASE("Conversion is not ambiguous", "[parser][conflicts]")
{
    CHECK(cIsParsed("void f() { if ((size_t)*nlines == 1); }"));
    CHECK(cIsParsed("int a = (type)&a;"));
    CHECK(cIsParsed("int a = (type)*a;"));
    CHECK(cIsParsed("int a = (type)+a;"));
    CHECK(cIsParsed("int a = (type)-a;"));
    CHECK(cIsParsed("int a = (type)~a;"));
}

TEST_CASE("Postponed nodes aren't lost on conflict resolution via merging",
          "[parser][postponed][conflicts]")
{
    Tree tree = makeTree(R"(
        struct s {
            // Comment1
            int b : 1; // Comment2
        };
    )");

    CHECK(findNode(tree, Type::Comments, "// Comment1") != nullptr);
}

TEST_CASE("sizeof() expr is resolved to a builtin type", "[parser][conflicts]")
{
    Tree tree = makeTree(R"(
        void f() {
            sizeof(int);
        }
    )");

    CHECK(findNode(tree, Type::Types, "int") != nullptr);
}

TEST_CASE("sizeof() is resolved to expression by default",
          "[parser][conflicts]")
{
    Tree tree = makeTree(R"(
        void f() {
            sizeof(var);
        }
    )");

    CHECK(findNode(tree, Type::Identifiers, "var") != nullptr);
}

TEST_CASE("Declaration/statement conflict is resolved as expected",
          "[parser][conflicts]")
{
    Tree tree = makeTree(R"(
        void f() {
            stmt;
            userType var;
            func(arg);
        }
    )");

    CHECK(findNode(tree, Type::Identifiers, "stmt") != nullptr);

    CHECK(findNode(tree, Type::UserTypes, "userType") != nullptr);
    CHECK(findNode(tree, Type::Identifiers, "var") != nullptr);

    CHECK(findNode(tree, Type::Functions, "func") != nullptr);
    CHECK(findNode(tree, Type::Identifiers, "arg") != nullptr);
}

TEST_CASE("Multi-line string literals are parsed", "[parser]")
{
    const char *const str = R"(
        const char *str = "\
        ";
    )";

    CHECK(cIsParsed(str));
}

TEST_CASE("Postponed nodes before string literals are preserved",
          "[parser][postponed]")
{
    Tree tree = makeTree(R"(
        const char *str = /*str*/ "";
    )");

    CHECK(findNode(tree, Type::Comments, "/*str*/") != nullptr);
}

TEST_CASE("Postponed nodes between string literals are preserved",
          "[parser][postponed]")
{
    Tree tree = makeTree(R"(
        const char *str = "" /*str*/ "";
    )");

    CHECK(printSubTree(*tree.getRoot(), true) ==
          "constchar*str=\"\"/*str*/\"\";");
}

TEST_CASE("Escaping of newline isn't rejected", "[parser]")
{
    const char *const str = R"(
        int \
            a;
    )";

    CHECK(cIsParsed(str));
}

TEST_CASE("Macros without args after function declaration are parsed",
          "[parser]")
{
    const char *const str =
        "char * format(const char fmt[], ...) _gnuc_printf;";
    CHECK(cIsParsed(str));
}

TEST_CASE("Macros with args after function declaration are parsed", "[parser]")
{
    const char *const str =
        "char * format(const char fmt[], ...) _gnuc_printf(1, 2);";
    CHECK(cIsParsed(str));
}

TEST_CASE("Function pointers returning user-defined types", "[parser]")
{
    CHECK(cIsParsed("typedef wint_t (*f)(int);"));
}

TEST_CASE("Attributes in typedef", "[parser]")
{
    const char *const str =
        "typedef union { int a, b; } __attribute__((packed)) u;";
    CHECK(cIsParsed(str));
}

TEST_CASE("Extra braces around call", "[parser][conflict]")
{
    CHECK(cIsParsed("int x = (U64)(func(a)) * b;"));
}

TEST_CASE("Types in macro arguments", "[parser][conflict]")
{
    CHECK(cIsParsed("int x = va_arg(ap, int);"));
}

TEST_CASE("Macro definition of function declaration", "[parser]")
{
    CHECK(cIsParsed("int DECL(func, (int arg), stuff);"));
}

TEST_CASE("asm directive", "[parser]")
{
    const char *const str = R"(
        void f() {
            __asm__("" ::: "memory");
        }
    )";

    CHECK(cIsParsed(str));
}

TEST_CASE("extern C", "[parser]")
{
    const char *const str = R"(
        void f() {
            __asm__ __volatile__("" ::: "memory");
        }
    )";

    CHECK(cIsParsed(R"(extern "C" { void f(); })"));
}

TEST_CASE("asm volatile directive", "[parser]")
{
    const char *const str = R"(
        void f() {
            __asm__ __volatile__("" ::: "memory");
        }
    )";

    CHECK(cIsParsed(str));
}

TEST_CASE("Trailing id in bitfield declarator is variable by default",
          "[parser][conflicts]")
{
    Tree tree = makeTree("struct s { int b : 1; };");
    CHECK(findNode(tree, Type::Identifiers, "b") != nullptr);
}

TEST_CASE("Single comment adds just one node to the tree",
          "[parser][postponed]")
{
    Tree withoutComment = makeTree(R"(
        void f()
        {
        }
    )");
    Tree withComment = makeTree(R"(
        // Comment
        void f()
        {
        }
    )");
    CHECK(countNodes(*withComment.getRoot()) ==
          countNodes(*withoutComment.getRoot()) + 2);
}

TEST_CASE("Floating point suffix isn't parsed standalone", "[parser]")
{
    CHECK(cIsParsed("float a = p+0;"));
    CHECK(cIsParsed("float a = p+0.5;"));
    CHECK(cIsParsed("double a = p+0;"));
    CHECK(cIsParsed("double a = p-0.1;"));
}

TEST_CASE("All forms of struct/union declarations are recognized", "[parser]")
{
    CHECK(cIsParsed("struct { };"));
    CHECK(cIsParsed("struct name { };"));
    CHECK(cIsParsed("struct name;"));
    CHECK(cIsParsed("union { };"));
    CHECK(cIsParsed("union name { };"));
    CHECK(cIsParsed("union name;"));
}

TEST_CASE("Parameter declaration can be followed by a macro",
          "[parser][extensions]")
{
    CHECK(cIsParsed("void f(char *name attr);"));
}

TEST_CASE("Function pointer can have its type modifiers",
          "[parser][conflicts][extensions]")
{
    CHECK(cIsParsed("int a = (LONG (WINAPI *)(HKEY))GPA();"));
    CHECK(cIsParsed("void (_cdecl *fptr)();"));
}

TEST_CASE("Single argument in parameter list is recognized as type",
          "[parser][conflicts][extensions]")
{
    // This should be a macro, because parameters must be named, but that's not
    // that important.
    Tree tree = makeTree("void f(C) { }");
    CHECK(findNode(tree, Type::Functions, "f") != nullptr);
    CHECK(findNode(tree, Type::UserTypes, "C") != nullptr);
}

TEST_CASE("Single argument function declarations", "[parser][extensions]")
{
    Tree tree;

    tree = makeTree("void func(type **arg);");
    CHECK(findNode(tree, Type::Functions, "func") != nullptr);
    CHECK(findNode(tree, Type::UserTypes, "type") != nullptr);
    CHECK(findNode(tree, Type::Identifiers, "arg") != nullptr);

    tree = makeTree("void func(const type *arg UNUSED);");
    CHECK(findNode(tree, Type::Functions, "func") != nullptr);
    CHECK(findNode(tree, Type::UserTypes, "type") != nullptr);
    CHECK(findNode(tree, Type::Identifiers, "arg") != nullptr);
    CHECK(findNode(tree, Type::Identifiers, "UNUSED") != nullptr);
}

TEST_CASE("Function name token is identified when followed with whitespace",
          "[parser]")
{
    Tree tree;

    tree = makeTree("void f();");
    CHECK(findNode(tree, Type::Functions, "f") != nullptr);

    tree = makeTree("void f ();");
    CHECK(findNode(tree, Type::Functions, "f") != nullptr);

    tree = makeTree("void f\n();");
    CHECK(findNode(tree, Type::Functions, "f") != nullptr);
}

TEST_CASE("Control-flow macros are allowed", "[parser][extensions]")
{
    const char *const str = R"(
        int f() {
            MACRO(arg) {
            }

            MACRO(arg)
                if (cond) action;

            switch (c){
                case 'P':
                    MACRO(arg)
                        if (cond)
                            break;
            }

            if (cond)
                MACRO(arg)
                    stmt;
        }
    )";

    CHECK(cIsParsed(str));
}

TEST_CASE("Comments don't affect resolution of declaration/statement conflict",
          "[parser][postponed][conflicts]")
{
    Tree tree;

    tree = makeTree("void f() { thisIsStatement; }");
    CHECK(findNode(tree, Type::Identifiers, "thisIsStatement") != nullptr);

    tree = makeTree("void f() { /* comment */ thisIsStatement; }");
    CHECK(findNode(tree, Type::Identifiers, "thisIsStatement") != nullptr);

    tree = makeTree(R"(
        void f() {
        #include "file.h"
            thisIsStatement;
        })"
    );
    CHECK(findNode(tree, Type::Identifiers, "thisIsStatement") != nullptr);
}

TEST_CASE("Single-parameter macro at global scope",
          "[parser][conflicts][extensions]")
{
    Tree tree = makeTree("DECLARE(rename_inside_subdir_ok);");
    CHECK(findNode(tree, Type::Functions, "DECLARE") != nullptr);
}

TEST_CASE("Multiple type specifiers inside structures", "[parser]")
{
    Tree tree = makeTree("struct name { unsigned int field; };", true);
    CHECK(findNode(tree, Type::Virtual, "unsignedintfield;") != nullptr);
}

static int
countNodes(const Node &root)
{
    int count = 0;

    std::function<void(const Node &)> visit = [&](const Node &node) {
        if (node.next != nullptr) {
            return visit(*node.next);
        }

        ++count;

        for (const Node *child : node.children) {
            visit(*child);
        }
    };

    visit(root);
    return count;
}
