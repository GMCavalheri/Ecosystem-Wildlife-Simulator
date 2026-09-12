#pragma once

#include <random>

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

// Phase 1: mean-field predator/prey interaction (no spatial grid yet). Kill count and
// resulting predator offspring are tau-leaped stochastic events approximating the
// Lotka-Volterra b*Prey*Predator and c*b*Prey*Predator terms. Spatial kill probability
// (predator hunger, prey density, relative GeneticTraits.speed) arrives in Phase 2+.
class PredationSystem {
public:
    void update(entt::registry& registry, std::mt19937& rng, float dt,
                const LotkaVolterraParams& params);
};

} // namespace eco
