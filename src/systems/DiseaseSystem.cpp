#include "systems/DiseaseSystem.h"

#include <algorithm>

#include "components/Components.h"
#include "components/PreyGroup.h"
#include "environment/Grid.h"

namespace eco {

void DiseaseSystem::update(entt::registry& registry, const Grid& grid, std::mt19937& rng,
                            float dt, const DiseaseParams& params) {
    const int width = grid.width();
    auto prey = preyGroup(registry);

    // Pass 1: how many infected prey does each cell hold?
    infectedPerCell_.assign(static_cast<std::size_t>(width) * grid.height(), 0);
    std::size_t totalInfected = 0;
    for (auto [entity, position, energy, traits, health, species] : prey.each()) {
        if (!health.infected) {
            continue;
        }
        ++infectedPerCell_[static_cast<std::size_t>(position.cellY) * width + position.cellX];
        ++totalInfected;
    }
    if (totalInfected == 0) {
        return;
    }

    // Infected prey leave the infectious state at the combined rate (recovery +
    // disease death); which outcome happens is decided by their relative share.
    const double totalLeaveRate =
        static_cast<double>(params.recoveryRate) + static_cast<double>(params.diseaseDeathRate);
    const double deathShare =
        totalLeaveRate > 0.0 ? static_cast<double>(params.diseaseDeathRate) / totalLeaveRate : 0.0;
    std::bernoulli_distribution leaves(std::clamp(totalLeaveRate * dt, 0.0, 1.0));
    std::bernoulli_distribution dies(deathShare);

    toRecover_.clear();
    toKill_.clear();

    // Pass 2: each prey is visited exactly once. Susceptibles are exposed to the
    // infected count their cell had at the start of the tick (so a newly infected prey
    // doesn't also progress in the same tick); infected prey progress.
    for (auto [entity, position, energy, traits, health, species] : prey.each()) {
        if (health.infected) {
            health.infectionTimer += dt;
            if (leaves(rng)) {
                (dies(rng) ? toKill_ : toRecover_).push_back(entity);
            }
        } else if (!health.immune) {
            const int infectedCellmates =
                infectedPerCell_[static_cast<std::size_t>(position.cellY) * width + position.cellX];
            if (infectedCellmates == 0) {
                continue;
            }
            const double p = std::clamp(
                static_cast<double>(params.transmissionRate) * infectedCellmates * dt, 0.0, 1.0);
            if (std::bernoulli_distribution(p)(rng)) {
                health.infected = true;
                health.infectionTimer = 0.0f;
            }
        }
    }

    for (auto entity : toRecover_) {
        auto& health = registry.get<Health>(entity);
        health.infected = false;
        health.immune = true;
        health.infectionTimer = 0.0f;
    }
    for (auto entity : toKill_) {
        registry.destroy(entity);
    }
}

} // namespace eco
