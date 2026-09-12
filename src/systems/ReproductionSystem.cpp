#include "systems/ReproductionSystem.h"

#include <algorithm>

#include "components/Components.h"

namespace eco {

void ReproductionSystem::update(entt::registry& registry, std::mt19937& rng, float dt,
                                 const LotkaVolterraParams& params) {
    std::size_t preyCount = 0;
    auto view = registry.view<Species>();
    for (auto entity : view) {
        if (view.get<Species>(entity).id == kPreySpeciesId) {
            ++preyCount;
        }
    }

    if (preyCount == 0) {
        return;
    }

    const double expectedBirths =
        static_cast<double>(params.preyBirthRate) * static_cast<double>(preyCount) *
        static_cast<double>(dt);
    std::poisson_distribution<int> birthDist(expectedBirths);
    int births = birthDist(rng);
    if (preyCount + static_cast<std::size_t>(births) > kMaxSpeciesPopulation) {
        births = static_cast<int>(kMaxSpeciesPopulation - std::min(preyCount, kMaxSpeciesPopulation));
    }

    for (int i = 0; i < births; ++i) {
        auto entity = registry.create();
        registry.emplace<Species>(entity, kPreySpeciesId);
    }
}

} // namespace eco
