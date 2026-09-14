#include "systems/MortalitySystem.h"

#include <algorithm>
#include <vector>

#include "components/Components.h"

namespace eco {

void MortalitySystem::update(entt::registry& registry, std::mt19937& rng, float dt,
                              const LotkaVolterraParams& params) {
    // Prey: starvation death once Energy is depleted (Phase 2's carrying-capacity
    // mechanism -- no vegetation locally means no energy means no survival).
    std::vector<entt::entity> starved;
    auto energyView = registry.view<Species, Energy>();
    for (auto entity : energyView) {
        if (energyView.get<Species>(entity).id == kPreySpeciesId &&
            energyView.get<Energy>(entity).value <= 0.0f) {
            starved.push_back(entity);
        }
    }
    for (auto entity : starved) {
        registry.destroy(entity);
    }

    // Predators: background natural death (the d*Predator term; still mean-field).
    std::vector<entt::entity> predators;
    auto view = registry.view<Species>();
    for (auto entity : view) {
        if (view.get<Species>(entity).id == kPredatorSpeciesId) {
            predators.push_back(entity);
        }
    }

    if (predators.empty()) {
        return;
    }

    const double expectedDeaths = static_cast<double>(params.predatorDeathRate) *
                                   static_cast<double>(predators.size()) *
                                   static_cast<double>(dt);
    std::poisson_distribution<int> deathDist(expectedDeaths);
    const int deaths = std::min<int>(deathDist(rng), static_cast<int>(predators.size()));

    std::shuffle(predators.begin(), predators.end(), rng);
    for (int i = 0; i < deaths; ++i) {
        registry.destroy(predators[i]);
    }
}

} // namespace eco
