#pragma once

#include <random>

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

// Phase 1: prey births as a tau-leaped Poisson process (the a*Prey term). Gating on
// Energy/Reproductive.readiness thresholds arrives once Foraging is wired in Phase 2.
class ReproductionSystem {
public:
    void update(entt::registry& registry, std::mt19937& rng, float dt,
                const LotkaVolterraParams& params);
};

} // namespace eco
