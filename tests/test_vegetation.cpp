#include <cmath>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/EcosystemParams.h"
#include "environment/Grid.h"
#include "systems/EnvironmentSystem.h"

// Phase 2 correctness gate: with seasonal variation disabled, per-cell vegetation
// regrowth is a pure logistic curve dV/dt = r*V*(1-V), which has the closed-form
// solution V(t) = 1 / (1 + ((1-V0)/V0) * exp(-r*t)). EnvironmentSystem integrates
// this numerically (explicit Euler); with a small dt it should track the analytic
// curve closely.
TEST_CASE("Vegetation regrowth matches the analytic logistic solution", "[vegetation]") {
    eco::VegetationParams params;
    params.seasonalAmplitude = 0.0f; // isolate the pure logistic dynamics
    params.baseTemperature = params.optimalTemperature; // suitability == 1 always

    eco::Grid grid(1, 1);
    grid.at(0, 0).vegetationDensity = 0.1f;
    const float v0 = grid.at(0, 0).vegetationDensity;

    eco::EnvironmentSystem system;
    constexpr float dt = 0.001f;
    constexpr float totalTime = 5.0f;
    constexpr int ticks = static_cast<int>(totalTime / dt);

    float simulationTime = 0.0f;
    for (int i = 0; i < ticks; ++i) {
        system.update(grid, dt, simulationTime, params);
        simulationTime += dt;
    }

    const float analytic =
        1.0f / (1.0f + ((1.0f - v0) / v0) * std::exp(-params.regrowthRate * totalTime));

    REQUIRE(grid.at(0, 0).vegetationDensity == Catch::Approx(analytic).margin(0.01));
}

TEST_CASE("Vegetation growth stalls far from the optimal temperature", "[vegetation]") {
    eco::VegetationParams params;
    params.seasonalAmplitude = 0.0f;
    params.baseTemperature = params.optimalTemperature + 10.0f * params.temperatureTolerance;

    eco::Grid grid(1, 1);
    grid.at(0, 0).vegetationDensity = 0.1f;
    const float v0 = grid.at(0, 0).vegetationDensity;

    eco::EnvironmentSystem system;
    for (int i = 0; i < 1000; ++i) {
        system.update(grid, 0.01f, 0.0f, params);
    }

    REQUIRE(grid.at(0, 0).vegetationDensity == Catch::Approx(v0).margin(0.001));
}

// Phase 8: with a per-cell capacity K and growth multiplier m the regrowth equation is
// dV/dt = r*m*V*(1 - V/K), whose closed form is
//   V(t) = K / (1 + ((K - V0)/V0) * exp(-r*m*t)).
// Same "compare the integrator to the exact solution" gate as the K=1 case, now proving
// the biome parameters enter the dynamics the way the math says they should.
TEST_CASE("Biome capacity and growth multiplier match the analytic logistic solution",
          "[vegetation][biomes]") {
    eco::VegetationParams params;
    params.seasonalAmplitude = 0.0f;
    params.baseTemperature = params.optimalTemperature;

    constexpr float capacity = 0.6f;
    constexpr float multiplier = 1.4f;
    constexpr float v0 = 0.05f;

    eco::Grid grid(1, 1);
    grid.at(0, 0).capacity = capacity;
    grid.at(0, 0).growthMultiplier = multiplier;
    grid.at(0, 0).vegetationDensity = v0;

    eco::EnvironmentSystem system;
    constexpr float dt = 0.001f;
    constexpr float totalTime = 6.0f;
    float simulationTime = 0.0f;
    for (int i = 0; i < static_cast<int>(totalTime / dt); ++i) {
        system.update(grid, dt, simulationTime, params);
        simulationTime += dt;
    }

    const float analytic =
        capacity /
        (1.0f + ((capacity - v0) / v0) * std::exp(-params.regrowthRate * multiplier * totalTime));
    REQUIRE(grid.at(0, 0).vegetationDensity == Catch::Approx(analytic).margin(0.005));
    REQUIRE(grid.at(0, 0).vegetationDensity <= capacity + 0.005f); // never overshoots K
}

// Phase 8 climate: temperature falls linearly with latitude (row), and the Gaussian
// suitability curve makes regrowth fastest in the row nearest the optimal temperature.
TEST_CASE("Regrowth is fastest in the latitude band nearest the optimal temperature",
          "[vegetation][seasons]") {
    eco::VegetationParams params;
    params.seasonalAmplitude = 0.0f;
    params.baseTemperature = params.optimalTemperature; // middle row sits exactly at optimum
    params.latitudeGradient = 30.0f;                    // +/-15 C from the middle to the edges

    eco::Grid grid(1, 21);
    for (int y = 0; y < 21; ++y) {
        grid.at(0, y).vegetationDensity = 0.1f;
    }
    eco::EnvironmentSystem system;
    for (int i = 0; i < 200; ++i) {
        system.update(grid, 0.01f, 0.0f, params);
    }

    // Symmetric about the middle row, strictly decreasing away from it.
    for (int y = 0; y < 10; ++y) {
        REQUIRE(grid.at(0, y).vegetationDensity < grid.at(0, y + 1).vegetationDensity);
        REQUIRE(grid.at(0, 20 - y).vegetationDensity < grid.at(0, 20 - y - 1).vegetationDensity);
        REQUIRE(grid.at(0, y).vegetationDensity ==
                Catch::Approx(grid.at(0, 20 - y).vegetationDensity).margin(1e-4));
    }
}

// The season slides that band up and down the grid: when the year is warm, the optimal
// temperature is reached at a cooler latitude (smaller y, since the bottom is warmer)
// and when it is cold, at a warmer one -- the moving target seasonal migrants chase.
TEST_CASE("The season moves the fastest-growing band up and down the grid",
          "[vegetation][seasons]") {
    eco::VegetationParams params;
    params.baseTemperature = params.optimalTemperature;
    params.seasonalAmplitude = 10.0f;
    params.seasonalPeriod = 40.0f;
    params.latitudeGradient = 40.0f;

    auto fastestRow = [&](float time) {
        eco::Grid grid(1, 41);
        for (int y = 0; y < 41; ++y) {
            grid.at(0, y).vegetationDensity = 0.1f;
        }
        eco::EnvironmentSystem system;
        system.update(grid, 0.01f, time, params);
        int best = 0;
        for (int y = 1; y < 41; ++y) {
            if (grid.at(0, y).vegetationDensity > grid.at(0, best).vegetationDensity) best = y;
        }
        return best;
    };

    const int warmSeason = fastestRow(10.0f);  // sin = +1: +10 C
    const int neutral = fastestRow(0.0f);      // sin = 0
    const int coldSeason = fastestRow(30.0f);  // sin = -1: -10 C

    REQUIRE(neutral == 20);            // middle row at the optimum
    REQUIRE(warmSeason < neutral);     // warm year: optimum is at the cooler (upper) rows
    REQUIRE(coldSeason > neutral);     // cold year: optimum is at the warmer (lower) rows
    REQUIRE(neutral - warmSeason == Catch::Approx(10.0 / 40.0 * 40.0).margin(1.0)); // dT / gradient * H
}
