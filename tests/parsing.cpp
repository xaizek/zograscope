#include "Catch/catch.hpp"

#include "TreeBuilder.hpp"
#include "parser.hpp"

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
