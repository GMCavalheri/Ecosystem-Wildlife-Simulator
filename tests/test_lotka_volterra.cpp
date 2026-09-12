#include <catch2/catch_test_macros.hpp>

#include "components/Components.h"
#include "core/Simulation.h"

// Phase 1 correctness gate: a mean-field (no grid) predator/prey pair, driven purely
// by stochastic tau-leaped birth/death/predation events, should reproduce classic
// Lotka-Volterra oscillation over a several-period window.
//
// Note: the underlying model has no carrying capacity, so it is only *neutrally*
// stable — demographic noise resonantly amplifies the cycles over long horizons and
// eventually drives one species extinct (a real, textbook property of individual-
// based LV systems, not a bug). This test window is chosen well within the range
// where that hasn't happened yet, verified empirically across many seeds.
TEST_CASE("Predator-prey populations oscillate without collapsing or exploding",
          "[lotka-volterra]") {
    eco::LotkaVolterraParams params; // a=1.0, b=0.005, c=0.5, d=0.8 -> prey*=320, pred*=200
    eco::Simulation sim(8, 8, params, /*rngSeed=*/1234u);

    sim.seedPopulation(eco::kPreySpeciesId, 280);
    sim.seedPopulation(eco::kPredatorSpeciesId, 170);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 600; // ~20 simulated time units, ~3 oscillation periods

    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
    }

    const auto& history = sim.metrics().history();
    REQUIRE(history.size() == static_cast<std::size_t>(ticks));

    std::size_t minPrey = history.front().preyCount, maxPrey = minPrey;
    std::size_t minPred = history.front().predatorCount, maxPred = minPred;
    for (const auto& snap : history) {
        minPrey = std::min(minPrey, snap.preyCount);
        maxPrey = std::max(maxPrey, snap.preyCount);
        minPred = std::min(minPred, snap.predatorCount);
        maxPred = std::max(maxPred, snap.predatorCount);
    }

    // Neither species collapses to extinction within this window...
    REQUIRE(minPrey > 20);
    REQUIRE(minPred > 10);

    // ...both visibly oscillate rather than sitting flat...
    REQUIRE(maxPrey > minPrey * 3 / 2);
    REQUIRE(maxPred > minPred * 3 / 2);

    // ...and neither runs away unbounded (would indicate a modeling/parameter bug).
    REQUIRE(maxPrey < 5000);
    REQUIRE(maxPred < 5000);
}
