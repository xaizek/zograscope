#include "Catch/catch.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include <functional>
#include <iostream>

#include "TreeBuilder.hpp"
#include "parser.hpp"
#include "tree.hpp"
#include "types.hpp"

#include "tests.hpp"

static int countNodes(const Node &root);

TEST_CASE("Empty input is OK", "[parser][extensions]")
{
    CHECK_FALSE(parse("").hasFailed());
    CHECK_FALSE(parse("      ").hasFailed());
    CHECK_FALSE(parse("\t\n \t \n").hasFailed());

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
    CHECK_FALSE(parse("int\r\na\r;\n").hasFailed());
}

TEST_CASE("Error message counts tabulation as single character", "[parser]")
{
    StreamCapture coutCapture(std::cout);
    CHECK(parse("\t\x01;").hasFailed());
    REQUIRE(boost::starts_with(coutCapture.get(), "<input>:1:2:"));
}

TEST_CASE("Top-level macros are parsed successfully", "[parser][extensions]")
{
    CHECK_FALSE(parse("TSTATIC_DEFS(int func(type *var);)").hasFailed());
    CHECK_FALSE(parse("ARRAY_GUARD(sort_enum, 1 + SK_COUNT);").hasFailed());
    CHECK_FALSE(parse("XX(state_t) st;").hasFailed());
    CHECK_FALSE(parse("MACRO();").hasFailed());
}

TEST_CASE("Empty initializer list is parsed", "[parser][extensions]")
{
    CHECK_FALSE(parse("args_t args = {};").hasFailed());
}

TEST_CASE("Bit field is parsed", "[parser][conflicts]")
{
    CHECK_FALSE(parse("struct s { unsigned int stuff : 1; };").hasFailed());
}

TEST_CASE("Conversion is not ambiguous", "[parser][conflicts]")
{
    CHECK_FALSE(parse("void f() { if ((size_t)*nlines == 1); }").hasFailed());
    CHECK_FALSE(parse("int a = (type)&a;").hasFailed());
    CHECK_FALSE(parse("int a = (type)*a;").hasFailed());
    CHECK_FALSE(parse("int a = (type)+a;").hasFailed());
    CHECK_FALSE(parse("int a = (type)-a;").hasFailed());
    CHECK_FALSE(parse("int a = (type)~a;").hasFailed());
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

    CHECK_FALSE(parse(str).hasFailed());
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

    CHECK(printSubTree(*tree.getRoot()) == "constchar*str=\"\"/*str*/\"\";");
}

TEST_CASE("Escaping of newline isn't rejected", "[parser]")
{
    const char *const str = R"(
        int \
            a;
    )";

    CHECK_FALSE(parse(str).hasFailed());
}

TEST_CASE("Macros without args after function declaration are parsed",
          "[parser]")
{
    const char *const str =
        "char * format(const char fmt[], ...) _gnuc_printf;";
    CHECK_FALSE(parse(str).hasFailed());
}

TEST_CASE("Macros with args after function declaration are parsed", "[parser]")
{
    const char *const str =
        "char * format(const char fmt[], ...) _gnuc_printf(1, 2);";
    CHECK_FALSE(parse(str).hasFailed());
}

TEST_CASE("Function pointers returning user-defined types", "[parser]")
{
    CHECK_FALSE(parse("typedef wint_t (*f)(int);").hasFailed());
}

TEST_CASE("Attributes in typedef", "[parser]")
{
    const char *const str =
        "typedef union { int a, b; } __attribute__((packed)) u;";
    CHECK_FALSE(parse(str).hasFailed());
}

TEST_CASE("Extra braces around call", "[parser][conflict]")
{
    CHECK_FALSE(parse("int x = (U64)(func(a)) * b;").hasFailed());
}

TEST_CASE("Types in macro arguments", "[parser][conflict]")
{
    CHECK_FALSE(parse("int x = va_arg(ap, int);").hasFailed());
}

TEST_CASE("Macro definition of function declaration", "[parser]")
{
    CHECK_FALSE(parse("int DECL(func, (int arg), stuff);").hasFailed());
}

TEST_CASE("asm directive", "[parser]")
{
    const char *const str = R"(
        void f() {
            __asm__("" ::: "memory");
        }
    )";

    CHECK_FALSE(parse(str).hasFailed());
}

TEST_CASE("extern C", "[parser]")
{
    const char *const str = R"(
        void f() {
            __asm__ __volatile__("" ::: "memory");
        }
    )";

    CHECK_FALSE(parse(R"(extern "C" { void f(); })").hasFailed());
}

TEST_CASE("asm volatile directive", "[parser]")
{
    const char *const str = R"(
        void f() {
            __asm__ __volatile__("" ::: "memory");
        }
    )";

    CHECK_FALSE(parse(str).hasFailed());
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
    CHECK_FALSE(parse("float a = p+0;").hasFailed());
    CHECK_FALSE(parse("float a = p+0.5;").hasFailed());
    CHECK_FALSE(parse("double a = p+0;").hasFailed());
    CHECK_FALSE(parse("double a = p-0.1;").hasFailed());
}

TEST_CASE("All forms of struct/union declarations are recognized", "[parser]")
{
    CHECK_FALSE(parse("struct { };").hasFailed());
    CHECK_FALSE(parse("struct name { };").hasFailed());
    CHECK_FALSE(parse("struct name;").hasFailed());
    CHECK_FALSE(parse("union { };").hasFailed());
    CHECK_FALSE(parse("union name { };").hasFailed());
    CHECK_FALSE(parse("union name;").hasFailed());
}

TEST_CASE("Parameter declaration can be followed by a macro",
          "[parser][extensions]")
{
    CHECK_FALSE(parse("void f(char *name attr);").hasFailed());
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
