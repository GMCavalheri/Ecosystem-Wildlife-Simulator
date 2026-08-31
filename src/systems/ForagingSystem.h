#pragma once

#include <entt/entt.hpp>

namespace eco {

class Grid;

// Herbivores consume local vegetation, gain Energy; depletes Cell.vegetationDensity.
class ForagingSystem {
public:
    void update(entt::registry& registry, Grid& grid);
};

} // namespace eco
