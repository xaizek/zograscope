#include "Catch/catch.hpp"

#include "TreeBuilder.hpp"
#include "parser.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Empty input is OK", "[parser][extensions]")
{
    CHECK_FALSE(parse("").hasFailed());
    CHECK_FALSE(parse("      ").hasFailed());
    CHECK_FALSE(parse("\t\n \t \n").hasFailed());
}

TEST_CASE("Top-level macros are parsed successfully", "[parser][extensions]")
{
    CHECK_FALSE(parse("TSTATIC_DEFS(int func(type *var);)").hasFailed());
    CHECK_FALSE(parse("ARRAY_GUARD(sort_enum, 1 + SK_COUNT);").hasFailed());
    CHECK_FALSE(parse("XX(state_t) st;").hasFailed());
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

TEST_CASE("Multi-line string literals are parsed", "[parser]")
{
    const char *const str = R"(
        const char *str = "\
        ";
    )";

    CHECK_FALSE(parse(str).hasFailed());
}

TEST_CASE("Macros after function declaration are parsed", "[parser]")
{
    const char *const str =
        "char * format(const char fmt[], ...) _gnuc_printf(1, 2);";
    CHECK_FALSE(parse(str).hasFailed());
}

TEST_CASE("Trailing id in bitfield declarator is variable by default",
          "[parser][conflicts]")
{
    Tree tree = makeTree("struct s { int b : 1; };");
    CHECK(findNode(tree, Type::Identifiers, "b") != nullptr);
}
