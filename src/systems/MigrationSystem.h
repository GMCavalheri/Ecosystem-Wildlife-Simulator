#pragma once

#include <random>

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

class Grid;
class ThreadPool;

// Phase 5: greedy gradient-following toward the neighboring cell with the best
// vegetation -- not full pathfinding. This is the real fix for the Phase 2/3 finding
// that a stationary population inevitably exhausts its own patch: a prey that can
// walk away from a depleted cell has somewhere to go.
class MigrationSystem {
public:
    // Prey are processed in fixed-size chunks (independent of thread count), each with
    // its own RNG seeded up front from `rng`, so the result is identical whether the
    // chunks run serially (pool == nullptr) or spread across any number of threads.
    // Safe to parallelize because each prey reads the (read-only) grid and writes only
    // its own Position.
    void update(entt::registry& registry, const Grid& grid, std::mt19937& rng, float dt,
                const MigrationParams& params, ThreadPool* pool = nullptr);
};

} // namespace eco
