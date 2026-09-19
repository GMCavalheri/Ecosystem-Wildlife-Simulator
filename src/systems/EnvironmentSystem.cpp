#include "systems/EnvironmentSystem.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/ThreadPool.h"
#include "environment/Grid.h"

namespace eco {

void EnvironmentSystem::update(Grid& grid, float dt, float simulationTime,
                                const VegetationParams& params, ThreadPool* pool) {
    constexpr float kTwoPi = 6.28318530718f;
    const float seasonalTemperature =
        params.baseTemperature +
        params.seasonalAmplitude * std::sin(kTwoPi * simulationTime / params.seasonalPeriod);

    // Climate depends only on the row (latitude) and the season, so the temperature and
    // the Gaussian suitability curve are computed once per row, not once per cell.
    const int height = grid.height();
    std::vector<float> rowTemperature(static_cast<std::size_t>(height));
    std::vector<float> rowRate(static_cast<std::size_t>(height));
    for (int y = 0; y < height; ++y) {
        const float latitude = height > 1 ? static_cast<float>(y) / (height - 1) - 0.5f : 0.0f;
        const float temperature = seasonalTemperature + params.latitudeGradient * latitude;
        const float offset = temperature - params.optimalTemperature;
        const float suitability =
            std::exp(-(offset * offset) /
                     (2.0f * params.temperatureTolerance * params.temperatureTolerance));
        rowTemperature[static_cast<std::size_t>(y)] = temperature;
        rowRate[static_cast<std::size_t>(y)] = params.regrowthRate * suitability;
    }

    constexpr int kRowsPerBlock = 16;
    const int blocks = (grid.height() + kRowsPerBlock - 1) / kRowsPerBlock;
    auto processBlock = [&](std::size_t block) {
        const int firstRow = static_cast<int>(block) * kRowsPerBlock;
        const int lastRow = std::min(firstRow + kRowsPerBlock, grid.height());
        for (int y = firstRow; y < lastRow; ++y) {
            const float temperature = rowTemperature[static_cast<std::size_t>(y)];
            const float effectiveRate = rowRate[static_cast<std::size_t>(y)];
            for (int x = 0; x < grid.width(); ++x) {
                Cell& cell = grid.at(x, y);
                cell.temperature = temperature;

                const float capacity = std::max(cell.capacity, 1e-3f);
                const float growth = effectiveRate * cell.growthMultiplier *
                                     cell.vegetationDensity *
                                     (1.0f - cell.vegetationDensity / capacity);
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
