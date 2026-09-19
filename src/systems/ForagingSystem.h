#pragma once

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

class Grid;

// Phase 2: herbivores consume vegetation from their own cell, gain Energy, and pay a
// constant metabolic cost -- the real, space-limited carrying-capacity mechanism.
class ForagingSystem {
public:
    // `competitorParams` applies to kCompetitorSpeciesId; everything else uses `params`.
    void update(entt::registry& registry, Grid& grid, float dt, const ForagingParams& params,
                const ForagingParams& competitorParams);
};

} // namespace eco
