#pragma once

#include <entt/entt.hpp>

namespace eco {

class Grid;

// Greedy gradient-following toward neighboring cells with a better resource/threat
// balance — not full pathfinding.
class MigrationSystem {
public:
    void update(entt::registry& registry, Grid& grid);
};

} // namespace eco
