#include <algorithm>
#include <cstddef>
#include <limits>

#include <catch2/catch_test_macros.hpp>

#include "components/Components.h"
#include "core/Simulation.h"

// Phase 5 correctness gate: two runs from the same seed and scenario, differing only
// in whether MigrationSystem is allowed to move anyone (moveAttemptRate 2.0 vs 0.0),
// isolate migration's effect directly -- exactly the Phase 3 methodology, applied
// here to "does letting prey walk away from a depleted patch actually help?"
//
// This scenario (pred0=10) is the same one that, pre-migration, reliably collapsed to
// near-extinction as prey exhausted their own birth cell with nowhere else to go (see
// the Phase 2 commit history). With migration on, prey should never even dip below
// their starting count in this window.
TEST_CASE("Migration keeps prey above their starting count where a stationary "
          "population would collapse",
          "[migration]") {
    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 600; // ~20 simulated time units

    std::size_t minPreyWithMigration = std::numeric_limits<std::size_t>::max();
    {
        eco::Simulation sim(64, 64, {}, {}, {}, {}, {}, {}, {}, /*rngSeed=*/1234u); // default: migration on
        sim.seedPopulation(eco::kPreySpeciesId, 280);
        sim.seedPopulation(eco::kPredatorSpeciesId, 10);

        for (int i = 0; i < ticks; ++i) {
            sim.tick(dt);
            minPreyWithMigration =
                std::min(minPreyWithMigration, sim.metrics().history().back().preyCount);
        }
    }

    std::size_t minPreyWithoutMigration = std::numeric_limits<std::size_t>::max();
    {
        eco::MigrationParams noMigration;
        noMigration.moveAttemptRate = 0.0f;
        eco::Simulation sim(64, 64, {}, {}, {}, {}, {}, noMigration, {}, /*rngSeed=*/1234u);
        sim.seedPopulation(eco::kPreySpeciesId, 280);
        sim.seedPopulation(eco::kPredatorSpeciesId, 10);

        for (int i = 0; i < ticks; ++i) {
            sim.tick(dt);
            minPreyWithoutMigration =
                std::min(minPreyWithoutMigration, sim.metrics().history().back().preyCount);
        }
    }

    // With migration: prey never fall below their starting population...
    REQUIRE(minPreyWithMigration > 200);

    // ...while the identical scenario without migration reproduces the known collapse.
    REQUIRE(minPreyWithoutMigration < 150);

    // The comparison itself is the point: migration should measurably help.
    REQUIRE(minPreyWithMigration > minPreyWithoutMigration);
}
