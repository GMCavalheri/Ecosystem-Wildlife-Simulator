#include <cstddef>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "components/Components.h"
#include "core/Simulation.h"

namespace {

// Mean prey speed at ~12 simulated time units, averaged over `seedCount` independent
// runs (seeds 100..100+seedCount-1) with predation either on or off.
double meanSpeedAtT12(bool predationOn, std::size_t seedCount) {
    double total = 0.0;
    for (unsigned seed = 100; seed < 100 + seedCount; ++seed) {
        eco::PredatorParams predatorParams; // b=0.002 -- predation pressure active
        if (!predationOn) {
            predatorParams.predationRate = 0.0f; // no selection pressure -- neutral drift only
        }
        eco::Simulation sim(64, 64, predatorParams, {}, {}, {}, {}, {}, {}, seed);
        sim.seedPopulation(eco::kPreySpeciesId, 280);
        sim.seedPopulation(eco::kPredatorSpeciesId, 30);

        constexpr float dt = 1.0f / 30.0f;
        constexpr int ticks = 360; // ~12 simulated time units: predators still numerous
        for (int i = 0; i < ticks; ++i) {
            sim.tick(dt);
        }
        total += sim.metrics().history().back().avgPreySpeed;
    }
    return total / static_cast<double>(seedCount);
}

} // namespace

// Phase 3 correctness gate: PredationSystem catches slow prey preferentially
// (catchability = 1/speed), and ReproductionSystem mutates GeneticTraits on birth.
// Founders start homogeneous at speed=1.0, so any shift in the population mean is a
// real, measurable response to selection -- the concrete example named in the project
// plan ("faster prey surviving more predation events").
//
// The effect is small per run (~+0.4% mean speed by t=12) and individual runs are noisy,
// so this averages 12 independent seeds per arm instead of trusting a single trajectory.
// (It originally asserted one seed's value; Phase 7's faster victim sampler consumes
// random numbers differently, which reshuffled that one trajectory and exposed the
// single-seed threshold as fragile -- the effect itself was unchanged: 40 seeds gave
// +1.0036 with predation vs 0.9996 without.)
TEST_CASE("Predation pressure raises mean prey speed above the neutral baseline",
          "[genetics]") {
    constexpr std::size_t seeds = 12;
    const double withPredation = meanSpeedAtT12(true, seeds);
    const double withoutPredation = meanSpeedAtT12(false, seeds);

    // Selection: faster on average with predation than without.
    REQUIRE(withPredation - withoutPredation > 0.002);

    // Control: mutation alone (no predation) leaves the mean essentially where it
    // started -- unbiased Gaussian noise has no reason to push it either way.
    REQUIRE(withoutPredation == Catch::Approx(1.0).margin(0.01));
}
