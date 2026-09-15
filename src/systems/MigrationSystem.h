#pragma once

#include <random>

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

class Grid;

// Phase 5: greedy gradient-following toward the neighboring cell with the best
// vegetation -- not full pathfinding. This is the real fix for the Phase 2/3 finding
// that a stationary population inevitably exhausts its own patch: a prey that can
// walk away from a depleted cell has somewhere to go.
class MigrationSystem {
public:
    void update(entt::registry& registry, Grid& grid, std::mt19937& rng, float dt,
                const MigrationParams& params);
};

} // namespace eco
