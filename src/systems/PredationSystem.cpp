#include "systems/PredationSystem.h"

#include <algorithm>
#include <vector>

#include "components/Components.h"

namespace eco {

void PredationSystem::update(entt::registry& registry, std::mt19937& rng, float dt,
                              const LotkaVolterraParams& params) {
    std::vector<entt::entity> prey;
    std::size_t predatorCount = 0;

    auto view = registry.view<Species>();
    for (auto entity : view) {
        if (view.get<Species>(entity).id == kPreySpeciesId) {
            prey.push_back(entity);
        } else if (view.get<Species>(entity).id == kPredatorSpeciesId) {
            ++predatorCount;
        }
    }

    if (prey.empty() || predatorCount == 0) {
        return;
    }

    const double expectedKills =
        static_cast<double>(params.predationRate) * static_cast<double>(prey.size()) *
        static_cast<double>(predatorCount) * static_cast<double>(dt);
    std::poisson_distribution<int> killDist(expectedKills);
    const int kills = std::min<int>(killDist(rng), static_cast<int>(prey.size()));

    std::shuffle(prey.begin(), prey.end(), rng);
    for (int i = 0; i < kills; ++i) {
        registry.destroy(prey[i]);
    }

    if (kills > 0) {
        std::poisson_distribution<int> birthDist(
            static_cast<double>(params.conversionEfficiency) * kills);
        int predatorBirths = birthDist(rng);
        if (predatorCount + static_cast<std::size_t>(predatorBirths) > kMaxSpeciesPopulation) {
            predatorBirths = static_cast<int>(
                kMaxSpeciesPopulation - std::min(predatorCount, kMaxSpeciesPopulation));
        }
        for (int i = 0; i < predatorBirths; ++i) {
            auto entity = registry.create();
            registry.emplace<Species>(entity, kPredatorSpeciesId);
        }
    }
}

} // namespace eco
