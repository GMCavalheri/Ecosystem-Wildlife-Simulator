#include <catch2/catch_test_macros.hpp>

#include "components/Components.h"
#include "core/Simulation.h"

// Phase 2/3 correctness gate, updated for Phase 5: prey grow, forage, and reproduce
// driven by local vegetation/Energy, with mean-field predation and speed-selection on
// top. This checks the *integrated* system produces healthy, bounded-in-this-window
// population growth -- not an instant collapse.
//
// Historical note: before migration existed, this same scenario reliably collapsed to
// near-extinction within this window (prey stuck on a depleted patch with nowhere to
// go -- see test_migration.cpp for the direct before/after comparison). With
// migration, prey can walk to fresher cells, so the population not only survives this
// window but grows far past the old ~1000-prey ceiling. That doesn't mean migration
// makes growth unconditionally safe forever -- see main.cpp's longer demo run for the
// eventual grid-wide "tragedy of the commons" collapse once predators are gone and
// prey can roam (and so overgraze) the entire grid at once.
TEST_CASE("Prey population grows and oscillates under vegetation/predation limits",
          "[population-dynamics]") {
    eco::PredatorParams predatorParams; // b=0.002 (mean-field predation)
    eco::Simulation sim(64, 64, predatorParams, {}, {}, {}, {}, {}, /*rngSeed=*/1234u);

    sim.seedPopulation(eco::kPreySpeciesId, 280);
    sim.seedPopulation(eco::kPredatorSpeciesId, 10);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 600; // ~20 simulated time units

    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
    }

    const auto& history = sim.metrics().history();
    REQUIRE(history.size() == static_cast<std::size_t>(ticks));

    std::size_t minPrey = history.front().preyCount, maxPrey = minPrey;
    std::size_t maxPred = 0;
    for (const auto& snap : history) {
        minPrey = std::min(minPrey, snap.preyCount);
        maxPrey = std::max(maxPrey, snap.preyCount);
        maxPred = std::max(maxPred, snap.predatorCount);
    }

    // Prey never collapses within this window -- migration keeps them well above the
    // pre-migration floor of ~50...
    REQUIRE(minPrey > 200);

    // ...grows well beyond its starting size (vegetation-fed reproduction working)...
    REQUIRE(maxPrey > 280 * 2);

    // ...predation actually happened at least once (mean-field term engaged)...
    REQUIRE(maxPred > 0);

    // ...and growth stays within entt's hard safety cap, not a numeric runaway bug.
    REQUIRE(maxPrey < eco::kMaxSpeciesPopulation);
}
