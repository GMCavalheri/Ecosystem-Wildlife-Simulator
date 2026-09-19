#include <cmath>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "components/Components.h"
#include "core/Simulation.h"

namespace {

struct SeasonalResponse {
    double amplitude = 0.0; // rows, oscillation of mean prey latitude at the season period
    double phase = 0.0;     // radians
};

// Runs a fixed-size herd (reproduction off, no predators, so nothing but movement
// changes the herd's latitude) in a world with the given latitude climate gradient and
// migration rate, and measures how strongly the herd's mean latitude oscillates at the
// seasonal frequency: the Fourier component of mean-y(t) at period P, over the last two
// full periods (after the initial transient).
SeasonalResponse measure(float latitudeGradient, float moveAttemptRate, unsigned seed) {
    constexpr float period = 40.0f;

    eco::PredatorParams noPredators;
    noPredators.predationRate = 0.0f;
    eco::VegetationParams vegetation;
    vegetation.latitudeGradient = latitudeGradient;
    vegetation.seasonalAmplitude = 10.0f;
    vegetation.seasonalPeriod = period;
    vegetation.temperatureTolerance = 6.0f; // a sharp growth band, so there is a gradient to follow
    eco::ReproductionParams noBirths;
    noBirths.attemptRate = 0.0f;
    eco::MigrationParams migration;
    migration.moveAttemptRate = moveAttemptRate;
    migration.minVegetationAdvantage = 0.02f;

    eco::Simulation sim(48, 48, noPredators, vegetation, {}, noBirths, {}, migration, {}, seed);
    sim.seedPopulation(eco::kPreySpeciesId, 900);

    double re = 0.0, im = 0.0;
    int samples = 0;
    for (int tick = 0; tick < 30 * 160; ++tick) {
        sim.tick(1.0f / 30.0f);
        const double t = sim.simulationTime();
        if (tick % 30 != 0 || t < 80.0) continue;

        double sumY = 0.0;
        int herd = 0;
        for (auto [entity, position] : sim.registry().view<const eco::Position>().each()) {
            sumY += position.cellY;
            ++herd;
        }
        if (herd == 0) return {};
        const double meanY = sumY / herd;
        const double w = 2.0 * 3.14159265358979 * t / period;
        re += meanY * std::cos(w);
        im += meanY * std::sin(w);
        ++samples;
    }
    return {2.0 * std::sqrt(re * re + im * im) / samples, std::atan2(im, re)};
}

} // namespace

// Phase 8: nothing here scripts a migration. The world has a latitude temperature
// gradient, so the season slides the band of fastest vegetation regrowth up and down the
// grid (test_vegetation.cpp checks that). Prey only ever do Phase 5's greedy step toward
// greener neighboring cells -- yet the herd should end up oscillating north and south
// with the seasons. Two controls show it takes both ingredients: without migration the
// herd can't follow the band, and without the gradient there is no band to follow.
TEST_CASE("Herds migrate seasonally with no migration script -- only with both a "
          "climate gradient and the ability to move",
          "[seasons][migration]") {
    std::vector<SeasonalResponse> both, noMigration, noGradient;
    for (unsigned seed : {1u, 2u, 3u}) {
        both.push_back(measure(30.0f, 2.0f, seed));
        noMigration.push_back(measure(30.0f, 0.0f, seed));
        noGradient.push_back(measure(0.0f, 2.0f, seed));
    }

    for (std::size_t i = 0; i < both.size(); ++i) {
        // A real seasonal swing (~2.7 rows measured)...
        REQUIRE(both[i].amplitude > 1.5);
        // ...an order of magnitude beyond either control (~0.15 and ~0.03 measured)...
        REQUIRE(both[i].amplitude > 8.0 * noMigration[i].amplitude);
        REQUIRE(both[i].amplitude > 8.0 * noGradient[i].amplitude);
        // ...and locked to the season: every seed lands on nearly the same phase, which
        // an incidental random walk would not do.
        REQUIRE(std::abs(both[i].phase - both[0].phase) < 0.3);
    }
}
