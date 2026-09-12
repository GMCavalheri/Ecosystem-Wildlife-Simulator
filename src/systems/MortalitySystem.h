#pragma once

#include <random>

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

// Phase 1: predator natural death as a tau-leaped Poisson process (the d*Predator
// term). Age/starvation/disease death arrive once those systems are implemented.
class MortalitySystem {
public:
    void update(entt::registry& registry, std::mt19937& rng, float dt,
                const LotkaVolterraParams& params);
};

} // namespace eco
