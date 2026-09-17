#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "components/Components.h"
#include "core/Simulation.h"

// Phase 3 correctness gate: PredationSystem now catches slow prey preferentially
// (catchability = 1/speed), and ReproductionSystem mutates GeneticTraits on birth.
// Founders start homogeneous at speed=1.0, so any shift in the population mean is a
// real, measurable response to selection -- this is the concrete example named in the
// project plan ("faster prey surviving more predation events").
//
// Two runs from the same seed, differing only in whether predation is switched on,
// isolate the effect: selection should measurably raise mean speed; with predation
// off, mutation alone should leave the mean essentially where it started (unbiased
// Gaussian noise has no reason to push it either way).
TEST_CASE("Predation pressure raises mean prey speed above the founder baseline",
          "[genetics]") {
    eco::PredatorParams predatorParams; // b=0.005 -- predation pressure active
    eco::Simulation sim(64, 64, predatorParams, {}, {}, {}, {}, {}, {}, /*rngSeed=*/1234u);

    sim.seedPopulation(eco::kPreySpeciesId, 280);
    sim.seedPopulation(eco::kPredatorSpeciesId, 30);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 360; // ~12 simulated time units: predators still numerous

    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
    }

    const auto& snap = sim.metrics().history().back();
    REQUIRE(snap.preyCount > 0);
    REQUIRE(snap.predatorCount > 0);
    REQUIRE(snap.avgPreySpeed > 1.005f);
}

TEST_CASE("Without predation, mutation alone does not bias mean prey speed",
          "[genetics]") {
    eco::PredatorParams predatorParams;
    predatorParams.predationRate = 0.0f; // no selection pressure -- neutral drift only
    eco::Simulation sim(64, 64, predatorParams, {}, {}, {}, {}, {}, {}, /*rngSeed=*/1234u);

    sim.seedPopulation(eco::kPreySpeciesId, 280);
    sim.seedPopulation(eco::kPredatorSpeciesId, 30);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 360;

    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
    }

    const auto& snap = sim.metrics().history().back();
    REQUIRE(snap.preyCount > 0);
    REQUIRE(snap.avgPreySpeed == Catch::Approx(1.0).margin(0.01));
}
