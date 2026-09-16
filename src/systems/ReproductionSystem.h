#pragma once

#include <random>

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

// Phase 2: prey whose Energy crosses a threshold reproduce at a constant per-tick
// probability, paying an Energy cost to spawn an offspring at their own cell.
// Phase 3: the offspring's GeneticTraits are the parent's plus mutation.
// Predator resilience fix: predators reproduce the same way (Energy-gated, no
// GeneticTraits/Position yet -- predation is still mean-field/non-spatial).
class ReproductionSystem {
public:
    void update(entt::registry& registry, std::mt19937& rng, float dt,
                const ReproductionParams& reproParams, const GeneticsParams& geneticsParams,
                const PredatorParams& predatorParams);
};

} // namespace eco
