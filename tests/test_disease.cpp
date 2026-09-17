#include <algorithm>
#include <cstddef>

#include <catch2/catch_test_macros.hpp>

#include "components/Components.h"
#include "core/Simulation.h"

namespace {

// Runs the outbreak in isolation on a single-cell (1x1) grid, which collapses
// DiseaseSystem's local, per-cell transmission down to the classical well-mixed SIR
// case: every prey shares the one cell, so "local infected neighbors" is just the
// population-wide infected count. Foraging metabolism and reproduction are switched
// off so starvation/births can't confound the S/I/R counts -- this isolates disease
// exactly the way test_vegetation.cpp isolated pure logistic growth by disabling
// seasonality.
std::size_t runOutbreak(float transmissionRate, unsigned rngSeed) {
    eco::DiseaseParams diseaseParams;
    diseaseParams.transmissionRate = transmissionRate;
    diseaseParams.recoveryRate = 0.3f;
    diseaseParams.diseaseDeathRate = 0.05f;

    eco::PredatorParams predatorParams;
    predatorParams.predationRate = 0.0f;
    eco::ForagingParams foragingParams;
    foragingParams.metabolicRate = 0.0f;
    eco::ReproductionParams reproParams;
    reproParams.attemptRate = 0.0f;

    eco::Simulation sim(1, 1, predatorParams, {}, foragingParams, reproParams, {}, {},
                         diseaseParams, rngSeed);
    sim.seedPopulation(eco::kPreySpeciesId, 200);
    sim.infectRandomPrey(5);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 900; // ~30 simulated time units: long enough to fully resolve

    std::size_t maxInfected = 5;
    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
        std::size_t infected = 0;
        auto view = sim.registry().view<const eco::Species, const eco::Health>();
        for (auto entity : view) {
            if (view.get<const eco::Species>(entity).id == eco::kPreySpeciesId &&
                view.get<const eco::Health>(entity).infected) {
                ++infected;
            }
        }
        maxInfected = std::max(maxInfected, infected);
    }
    return maxInfected;
}

} // namespace

// Phase 4 correctness gate: the classical SIR epidemic threshold theorem says an
// outbreak only takes off if R0 > 1, where R0 is the expected number of secondary
// infections one case produces in a fully susceptible population. Our transmission is
// mass-action (each infected cellmate independently contributes a fixed per-tick
// hazard to each susceptible, not normalized by local population size -- see
// DiseaseParams), so the threshold here is R0 = transmissionRate * S0 / (recoveryRate
// + diseaseDeathRate), not the frequency-normalized beta/gamma from the textbook
// per-capita form. With S0=195 (200 seeded minus 5 patient-zeros), gamma+mu=0.35:
//   supercritical: transmissionRate=0.01   -> R0 ~ 5.6  (epidemic should take off)
//   subcritical:   transmissionRate=0.0005 -> R0 ~ 0.28 (should fizzle immediately)
TEST_CASE("An outbreak takes off when R0 > 1 and fizzles when R0 < 1", "[disease]") {
    const std::size_t supercriticalPeak = runOutbreak(0.01f, /*rngSeed=*/1234u);
    const std::size_t subcriticalPeak = runOutbreak(0.0005f, /*rngSeed=*/1234u);

    // Supercritical: infected count grows far past the 5 patient-zeros...
    REQUIRE(supercriticalPeak > 50);

    // ...while subcritical never really takes off...
    REQUIRE(subcriticalPeak < 20);

    // ...and the comparison itself is the point: R0 crossing 1 is what matters.
    REQUIRE(supercriticalPeak > subcriticalPeak * 5);
}
