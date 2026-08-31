#include <catch2/catch_test_macros.hpp>

#include "environment/Grid.h"

TEST_CASE("Grid indexes cells by row-major coordinates", "[grid]") {
    eco::Grid grid(4, 3);

    REQUIRE(grid.width() == 4);
    REQUIRE(grid.height() == 3);

    grid.at(2, 1).vegetationDensity = 0.5f;

    REQUIRE(grid.at(2, 1).vegetationDensity == 0.5f);
    REQUIRE(grid.at(0, 0).vegetationDensity == 1.0f);
}
