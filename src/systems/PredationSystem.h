#pragma once

#include <random>

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

// Kill count is still a tau-leaped stochastic event approximating the Lotka-Volterra
// b*Prey*Predator term (predation itself is mean-field, not spatial). Phase 3: *which*
// prey get killed is not uniform -- catchability scales as 1/speed. Predator
// resilience fix: a kill's energy now feeds one existing predator (who reproduces
// individually via ReproductionSystem once fed enough) instead of instantly spawning a
// new predator population-wide.
class PredationSystem {
public:
    void update(entt::registry& registry, std::mt19937& rng, float dt,
                const PredatorParams& params);
};

} // namespace eco
