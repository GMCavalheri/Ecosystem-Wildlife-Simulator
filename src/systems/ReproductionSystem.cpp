#include "systems/ReproductionSystem.h"

#include <algorithm>
#include <vector>

#include "components/Components.h"

namespace eco {

void ReproductionSystem::update(entt::registry& registry, std::mt19937& rng, float dt,
                                 const ReproductionParams& params) {
    std::vector<entt::entity> eligible;
    auto view = registry.view<Species, Energy>();
    for (auto entity : view) {
        if (view.get<Species>(entity).id == kPreySpeciesId &&
            view.get<Energy>(entity).value >= params.energyThreshold) {
            eligible.push_back(entity);
        }
    }

    std::bernoulli_distribution attempt(
        std::clamp(static_cast<double>(params.attemptRate) * dt, 0.0, 1.0));

    for (auto entity : eligible) {
        if (!attempt(rng)) {
            continue;
        }

        auto& parentEnergy = registry.get<Energy>(entity);
        if (parentEnergy.value < params.parentEnergyCost) {
            continue;
        }
        parentEnergy.value -= params.parentEnergyCost;

        const auto parentPosition = registry.get<Position>(entity);
        auto offspring = registry.create();
        registry.emplace<Species>(offspring, kPreySpeciesId);
        registry.emplace<Position>(offspring, parentPosition);
        registry.emplace<Energy>(offspring, params.offspringEnergy, 100.0f);
    }
}

} // namespace eco
