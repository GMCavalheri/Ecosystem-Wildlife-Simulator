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
