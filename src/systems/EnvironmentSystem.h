#pragma once

#include "core/EcosystemParams.h"

namespace eco {

class Grid;

// Phase 2: per-cell vegetation regrowth (logistic growth) under a seasonally-cycling
// temperature.
class EnvironmentSystem {
public:
    void update(Grid& grid, float dt, float simulationTime, const VegetationParams& params);
};

} // namespace eco
