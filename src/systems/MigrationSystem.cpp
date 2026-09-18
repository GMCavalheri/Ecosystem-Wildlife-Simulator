#include "systems/MigrationSystem.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "components/Components.h"
#include "components/PreyGroup.h"
#include "core/ThreadPool.h"
#include "environment/Grid.h"

namespace eco {

namespace {

constexpr std::size_t kChunkSize = 4096;

constexpr std::array<std::pair<int, int>, 8> kMooreOffsets = {{
    {-1, -1}, {0, -1}, {1, -1},
    {-1, 0},           {1, 0},
    {-1, 1},  {0, 1},  {1, 1},
}};

} // namespace

void MigrationSystem::update(entt::registry& registry, const Grid& grid, std::mt19937& rng,
                              float dt, const MigrationParams& params, ThreadPool* pool) {
    auto prey = preyGroup(registry);
    if (prey.size() == 0) {
        return;
    }

    // Group iterators are forward-only, so walk once (cheap) and remember where every
    // kChunkSize-th prey starts; each chunk then resumes from its saved iterator.
    auto range = prey.each();
    using Iterator = decltype(range.begin());
    std::vector<Iterator> chunkStarts;
    std::size_t walked = 0;
    for (auto it = range.begin(); it != range.end(); ++it, ++walked) {
        if (walked % kChunkSize == 0) {
            chunkStarts.push_back(it);
        }
    }
    const std::size_t total = walked;

    std::vector<std::uint32_t> chunkSeeds(chunkStarts.size());
    for (auto& seed : chunkSeeds) {
        seed = static_cast<std::uint32_t>(rng());
    }

    const double attemptProbability =
        std::clamp(static_cast<double>(params.moveAttemptRate) * dt, 0.0, 1.0);

    auto processChunk = [&](std::size_t chunk) {
        std::mt19937 chunkRng(chunkSeeds[chunk]);
        std::bernoulli_distribution attempt(attemptProbability);
        std::size_t remaining = std::min(kChunkSize, total - chunk * kChunkSize);

        for (Iterator it = chunkStarts[chunk]; remaining > 0; ++it, --remaining) {
            if (!attempt(chunkRng)) {
                continue;
            }
            auto [entity, position, energy, traits, health] = *it;

            const float currentVegetation =
                grid.at(position.cellX, position.cellY).vegetationDensity;

            float bestVegetation = currentVegetation;
            // At most 8 neighbors -- a fixed array avoids a heap allocation per moving prey.
            std::array<std::pair<int, int>, 8> bestCells;
            std::size_t bestCount = 0;

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
                    bestCount = 0;
                    bestCells[bestCount++] = {nx, ny};
                } else if (std::abs(neighborVegetation - bestVegetation) <= kTieEpsilon) {
                    bestCells[bestCount++] = {nx, ny};
                }
            }

            if (bestCount == 0 ||
                bestVegetation < currentVegetation + params.minVegetationAdvantage) {
                continue;
            }

            std::uniform_int_distribution<std::size_t> pick(0, bestCount - 1);
            const auto [nx, ny] = bestCells[pick(chunkRng)];
            position.cellX = nx;
            position.cellY = ny;
        }
    };

    if (pool != nullptr) {
        pool->parallelFor(chunkStarts.size(), processChunk);
    } else {
        for (std::size_t chunk = 0; chunk < chunkStarts.size(); ++chunk) {
            processChunk(chunk);
        }
    }
}

} // namespace eco
