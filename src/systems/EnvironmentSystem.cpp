#include "systems/EnvironmentSystem.h"

#include <algorithm>
#include <cmath>

#include "core/ThreadPool.h"
#include "environment/Grid.h"

namespace eco {

void EnvironmentSystem::update(Grid& grid, float dt, float simulationTime,
                                const VegetationParams& params, ThreadPool* pool) {
    constexpr float kTwoPi = 6.28318530718f;
    const float temperature =
        params.baseTemperature +
        params.seasonalAmplitude * std::sin(kTwoPi * simulationTime / params.seasonalPeriod);

    const float offset = temperature - params.optimalTemperature;
    const float suitability = std::exp(
        -(offset * offset) / (2.0f * params.temperatureTolerance * params.temperatureTolerance));
    const float effectiveRate = params.regrowthRate * suitability;

    constexpr int kRowsPerBlock = 16;
    const int blocks = (grid.height() + kRowsPerBlock - 1) / kRowsPerBlock;
    auto processBlock = [&](std::size_t block) {
        const int firstRow = static_cast<int>(block) * kRowsPerBlock;
        const int lastRow = std::min(firstRow + kRowsPerBlock, grid.height());
        for (int y = firstRow; y < lastRow; ++y) {
            for (int x = 0; x < grid.width(); ++x) {
                Cell& cell = grid.at(x, y);
                cell.temperature = temperature;

                const float growth =
                    effectiveRate * cell.vegetationDensity * (1.0f - cell.vegetationDensity);
                cell.vegetationDensity =
                    std::clamp(cell.vegetationDensity + growth * dt, 0.0f, 1.0f);
            }
        }
    };

    if (pool != nullptr) {
        pool->parallelFor(static_cast<std::size_t>(blocks), processBlock);
    } else {
        for (int block = 0; block < blocks; ++block) {
            processBlock(static_cast<std::size_t>(block));
        }
    }
}

} // namespace eco
