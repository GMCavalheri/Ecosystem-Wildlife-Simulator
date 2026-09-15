#pragma once

#include <random>

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

// Kill count and resulting predator offspring are tau-leaped stochastic events
// approximating the Lotka-Volterra b*Prey*Predator and c*b*Prey*Predator terms
// (predation itself is still mean-field, not spatial). Phase 3: *which* prey get
// killed is no longer uniform -- catchability scales as 1/speed, so
// GeneticTraits.speed is a real, measurable selection pressure.
class PredationSystem {
public:
    void update(entt::registry& registry, std::mt19937& rng, float dt,
                const LotkaVolterraParams& params);
};

} // namespace eco
