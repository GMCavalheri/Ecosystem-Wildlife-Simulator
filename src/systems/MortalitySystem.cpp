#include "systems/MortalitySystem.h"

#include <vector>

#include "components/Components.h"

namespace eco {

void MortalitySystem::update(entt::registry& registry) {
    // Both prey and predators now share the same starvation rule: Energy depleted to
    // zero means death. Collect first, then destroy, since destroying while iterating
    // the view is unsafe.
    std::vector<entt::entity> starved;
    auto view = registry.view<const Energy>();
    for (auto entity : view) {
        if (view.get<const Energy>(entity).value <= 0.0f) {
            starved.push_back(entity);
        }
    }
    for (auto entity : starved) {
        registry.destroy(entity);
    }
}

} // namespace eco
