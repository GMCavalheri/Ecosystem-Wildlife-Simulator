#include "systems/EnvironmentSystem.h"

#include <algorithm>
#include <cmath>

#include "environment/Grid.h"

namespace eco {

void EnvironmentSystem::update(Grid& grid, float dt, float simulationTime,
                                const VegetationParams& params) {
    constexpr float kTwoPi = 6.28318530718f;
    const float temperature =
        params.baseTemperature +
        params.seasonalAmplitude * std::sin(kTwoPi * simulationTime / params.seasonalPeriod);

    const float offset = temperature - params.optimalTemperature;
    const float suitability = std::exp(
        -(offset * offset) / (2.0f * params.temperatureTolerance * params.temperatureTolerance));
    const float effectiveRate = params.regrowthRate * suitability;

    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            Cell& cell = grid.at(x, y);
            cell.temperature = temperature;

            const float growth = effectiveRate * cell.vegetationDensity * (1.0f - cell.vegetationDensity);
            cell.vegetationDensity = std::clamp(cell.vegetationDensity + growth * dt, 0.0f, 1.0f);
        }
    }
}

} // namespace eco
