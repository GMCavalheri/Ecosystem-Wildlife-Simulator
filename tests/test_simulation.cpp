#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/Simulation.h"

TEST_CASE("Simulation advances time each tick", "[simulation]") {
    eco::Simulation sim(8, 8);

    REQUIRE(sim.simulationTime() == 0.0f);

    sim.tick(0.5f);
    sim.tick(0.5f);

    REQUIRE(sim.simulationTime() == Catch::Approx(1.0f));
}
