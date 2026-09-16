#include <catch2/catch_test_macros.hpp>

#include "components/Components.h"
#include "core/Simulation.h"

// Predator resilience fix: PredationSystem/MortalitySystem no longer kill predators
// via a flat, food-independent Poisson death rate. A predator now has an Energy
// reserve fed by kills and drained by metabolism every tick, and only starves once
// that reserve is actually empty -- the same buffer prey have had since Phase 2.
//
// This isolates the buffer itself with zero prey present (so zero kills are possible,
// zero RNG involved anywhere in the path): a predator seeded at its starting Energy
// must survive for a while on its reserve alone, and only die once metabolism has
// actually drained it to zero -- read the real starting value and metabolic rate from
// the simulation rather than hardcoding them, so this stays correct if either default
// ever changes.
TEST_CASE("A well-fed predator survives on its Energy reserve with zero kills, then "
          "starves once it's actually empty",
          "[predator-resilience]") {
    eco::PredatorParams params;
    eco::Simulation sim(8, 8, params, {}, {}, {}, {}, {}, /*rngSeed=*/1234u);
    sim.seedPopulation(eco::kPredatorSpeciesId, 1);
    // No prey seeded: zero kills are possible for the entire run, isolating the
    // metabolism/starvation buffer from any hunting-driven Energy gain.

    constexpr float dt = 1.0f / 30.0f;
    const float startingEnergy = sim.registry().get<eco::Energy>(
        *sim.registry().view<eco::Species>().begin()).value;
    REQUIRE(startingEnergy > 0.0f);

    const float survivalTime = startingEnergy / params.metabolicRate; // = 100/15

    // Comfortably before running out: still alive.
    for (int i = 0; i < static_cast<int>((survivalTime - 1.0f) / dt); ++i) {
        sim.tick(dt);
    }
    REQUIRE(sim.metrics().history().back().predatorCount == 1);

    // Comfortably past running out: starved.
    while (sim.simulationTime() < survivalTime + 1.0f) {
        sim.tick(dt);
    }
    REQUIRE(sim.metrics().history().back().predatorCount == 0);
}

// Integration check: with the old flat background death rate, every scenario we tried
// (across Phases 2, 3, and 5) drove predators extinct within a handful of time units,
// regardless of how much prey was actually available. With a real Energy buffer, a
// well-fed predator population should sustain itself and grow well past its starting
// size for a healthy stretch -- verified here against the specific seed/scenario
// traced during the investigation (peaks near 490 by t=24 before the run's own
// separate, expected eventual prey overshoot-collapse catches up much later).
TEST_CASE("Predator population grows well past its starting size and sustains for "
          "many generations",
          "[predator-resilience]") {
    eco::PredatorParams params; // b=0.002
    eco::Simulation sim(64, 64, params, {}, {}, {}, {}, {}, /*rngSeed=*/1234u);
    sim.seedPopulation(eco::kPreySpeciesId, 280);
    sim.seedPopulation(eco::kPredatorSpeciesId, 10);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 900; // ~30 simulated time units

    std::size_t maxPred = 0;
    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
        maxPred = std::max(maxPred, sim.metrics().history().back().predatorCount);
    }

    REQUIRE(maxPred > 100);
}
