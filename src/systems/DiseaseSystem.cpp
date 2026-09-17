#include "systems/DiseaseSystem.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "components/Components.h"

namespace eco {

namespace {

struct CellBucket {
    std::vector<entt::entity> susceptible;
    int infectedCount = 0;
};

long long cellKey(const Position& position) {
    return static_cast<long long>(position.cellY) * 1'000'000LL + position.cellX;
}

} // namespace

void DiseaseSystem::update(entt::registry& registry, std::mt19937& rng, float dt,
                            const DiseaseParams& params) {
    // Group prey by cell so transmission only happens between cellmates.
    std::unordered_map<long long, CellBucket> cells;
    auto view = registry.view<Species, Position, Health>();
    for (auto entity : view) {
        if (view.get<Species>(entity).id != kPreySpeciesId) {
            continue;
        }
        const auto& health = view.get<Health>(entity);
        auto& bucket = cells[cellKey(view.get<Position>(entity))];
        if (health.infected) {
            ++bucket.infectedCount;
        } else if (!health.immune) {
            bucket.susceptible.push_back(entity);
        }
    }

    // Transmission: each susceptible's infection risk this tick scales with how many
    // infected cellmates it has.
    for (auto& [key, bucket] : cells) {
        if (bucket.infectedCount == 0 || bucket.susceptible.empty()) {
            continue;
        }
        const double p = std::clamp(static_cast<double>(params.transmissionRate) *
                                         bucket.infectedCount * dt,
                                     0.0, 1.0);
        std::bernoulli_distribution infects(p);
        for (auto entity : bucket.susceptible) {
            if (infects(rng)) {
                auto& health = registry.get<Health>(entity);
                health.infected = true;
                health.infectionTimer = 0.0f;
            }
        }
    }

    // Progression: infected prey leave the infectious state at the combined rate
    // (recovery + disease death); which outcome happens is decided by their relative
    // share of that combined rate.
    const double totalLeaveRate =
        static_cast<double>(params.recoveryRate) + static_cast<double>(params.diseaseDeathRate);
    const double deathShare =
        totalLeaveRate > 0.0 ? static_cast<double>(params.diseaseDeathRate) / totalLeaveRate : 0.0;

    std::vector<entt::entity> toRecover;
    std::vector<entt::entity> toKill;

    for (auto entity : view) {
        if (view.get<Species>(entity).id != kPreySpeciesId) {
            continue;
        }
        auto& health = view.get<Health>(entity);
        if (!health.infected) {
            continue;
        }
        health.infectionTimer += dt;

        std::bernoulli_distribution leaves(std::clamp(totalLeaveRate * dt, 0.0, 1.0));
        if (!leaves(rng)) {
            continue;
        }

        std::bernoulli_distribution dies(deathShare);
        if (dies(rng)) {
            toKill.push_back(entity);
        } else {
            toRecover.push_back(entity);
        }
    }

    for (auto entity : toRecover) {
        auto& health = registry.get<Health>(entity);
        health.infected = false;
        health.immune = true;
        health.infectionTimer = 0.0f;
    }
    for (auto entity : toKill) {
        registry.destroy(entity);
    }
}

} // namespace eco
