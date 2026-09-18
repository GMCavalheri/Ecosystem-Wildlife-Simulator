#include "systems/ForagingSystem.h"

#include <algorithm>

#include "components/Components.h"
#include "components/PreyGroup.h"
#include "environment/Grid.h"

namespace eco {

void ForagingSystem::update(entt::registry& registry, Grid& grid, float dt,
                             const ForagingParams& params) {
    // Prey only (see PreyGroup.h): a linear walk over contiguous component arrays.
    for (auto [entity, position, energy, traits, health] : preyGroup(registry).each()) {
        Cell& cell = grid.at(position.cellX, position.cellY);

        const float wanted = params.maxIntakeRate * dt;
        const float energyRoom = (energy.max - energy.value) / params.energyPerVegetation;
        const float eaten = std::max(0.0f, std::min({wanted, cell.vegetationDensity, energyRoom}));

        cell.vegetationDensity -= eaten;
        energy.value = std::min(energy.max, energy.value + eaten * params.energyPerVegetation);
        energy.value = std::max(0.0f, energy.value - params.metabolicRate * dt);
    }
}

} // namespace eco
