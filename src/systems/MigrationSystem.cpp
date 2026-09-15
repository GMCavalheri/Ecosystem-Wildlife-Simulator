#include "systems/MigrationSystem.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "components/Components.h"
#include "environment/Grid.h"

namespace eco {

namespace {

constexpr std::array<std::pair<int, int>, 8> kMooreOffsets = {{
    {-1, -1}, {0, -1}, {1, -1},
    {-1, 0},           {1, 0},
    {-1, 1},  {0, 1},  {1, 1},
}};

} // namespace

void MigrationSystem::update(entt::registry& registry, Grid& grid, std::mt19937& rng,
                              float dt, const MigrationParams& params) {
    std::bernoulli_distribution attempt(
        std::clamp(static_cast<double>(params.moveAttemptRate) * dt, 0.0, 1.0));

    auto view = registry.view<Species, Position>();
    for (auto entity : view) {
        if (view.get<Species>(entity).id != kPreySpeciesId) {
            continue;
        }
        if (!attempt(rng)) {
            continue;
        }

        auto& position = view.get<Position>(entity);
        const float currentVegetation = grid.at(position.cellX, position.cellY).vegetationDensity;

        float bestVegetation = currentVegetation;
        std::vector<std::pair<int, int>> bestCells;

        for (const auto& [dx, dy] : kMooreOffsets) {
            const int nx = position.cellX + dx;
            const int ny = position.cellY + dy;
            if (nx < 0 || nx >= grid.width() || ny < 0 || ny >= grid.height()) {
                continue;
            }

            constexpr float kTieEpsilon = 1e-6f;
            const float neighborVegetation = grid.at(nx, ny).vegetationDensity;
            if (neighborVegetation > bestVegetation + kTieEpsilon) {
                bestVegetation = neighborVegetation;
                bestCells.clear();
                bestCells.emplace_back(nx, ny);
            } else if (std::abs(neighborVegetation - bestVegetation) <= kTieEpsilon) {
                bestCells.emplace_back(nx, ny);
            }
        }

        if (bestCells.empty() || bestVegetation < currentVegetation + params.minVegetationAdvantage) {
            continue;
        }

        std::uniform_int_distribution<std::size_t> pick(0, bestCells.size() - 1);
        const auto [nx, ny] = bestCells[pick(rng)];
        position.cellX = nx;
        position.cellY = ny;
    }
}

} // namespace eco
