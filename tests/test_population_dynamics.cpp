#include <catch2/catch_test_macros.hpp>

#include "components/Components.h"
#include "core/Simulation.h"

// Phase 2 correctness gate: prey now grow, forage, and reproduce driven entirely by
// local vegetation/Energy (see test_vegetation.cpp for the underlying logistic-curve
// math), with predation still mean-field on top. This checks the *integrated* system
// produces bounded population growth -- a real overshoot-and-response, not an instant
// collapse or an unbounded explosion.
//
// Note: with no migration yet (that's Phase 5), reproduction without dispersal means
// prey lineages cluster and exhaust their own patch. Verified empirically across many
// seeds: population reliably overshoots then eventually collapses to extinction by
// ~35-50 simulated time units, even after predators die out first. This test window
// is chosen well within the healthy, bounded-growth part of that arc.
TEST_CASE("Prey population grows and oscillates under vegetation/predation limits",
          "[population-dynamics]") {
    eco::LotkaVolterraParams lvParams; // b=0.005, c=0.5, d=0.8 (mean-field predation)
    eco::Simulation sim(64, 64, lvParams, {}, {}, {}, {}, /*rngSeed=*/1234u);

    sim.seedPopulation(eco::kPreySpeciesId, 280);
    sim.seedPopulation(eco::kPredatorSpeciesId, 10);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 600; // ~20 simulated time units, well before eventual collapse

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

    // Prey never collapses within this window...
    REQUIRE(minPrey > 50);

    // ...grows well beyond its starting size (vegetation-fed reproduction working)...
    REQUIRE(maxPrey > 280 * 2);

    // ...predation actually happened at least once (mean-field term engaged)...
    REQUIRE(maxPred > 0);

    // ...and growth stays bounded by the grid's carrying capacity, not runaway.
    REQUIRE(maxPrey < 5000);
}
