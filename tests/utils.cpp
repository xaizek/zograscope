#include "Catch/catch.hpp"

#include "utils.hpp"

TEST_CASE("Different strings are recognized as different", "[utils][dice]")
{
    REQUIRE(diceCoefficient("abc", "abd") < 1.0f);
}
