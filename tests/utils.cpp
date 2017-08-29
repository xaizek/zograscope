#include "Catch/catch.hpp"

#include "utils.hpp"

TEST_CASE("Different strings are recognized as different", "[utils][dice]")
{
    DiceString diceB("abd");
    REQUIRE(DiceString("abc").compare(diceB) < 1.0f);
}
