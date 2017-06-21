#include "Catch/catch.hpp"

#include "TreeBuilder.hpp"
#include "parser.hpp"

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
