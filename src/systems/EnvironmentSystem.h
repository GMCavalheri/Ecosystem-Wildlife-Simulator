#pragma once

#include "core/EcosystemParams.h"

namespace eco {

class Grid;
class ThreadPool;

// Phase 2: per-cell vegetation regrowth (logistic growth) under a seasonally-cycling
// temperature.
class EnvironmentSystem {
public:
    // Each cell evolves independently, so rows can be split across threads (pool !=
    // nullptr) with bit-identical results.
    void update(Grid& grid, float dt, float simulationTime, const VegetationParams& params,
                ThreadPool* pool = nullptr);
};

} // namespace eco
