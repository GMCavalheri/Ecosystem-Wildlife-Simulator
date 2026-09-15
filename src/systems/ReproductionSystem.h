#pragma once

#include <random>

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

// Phase 2: prey whose Energy crosses a threshold reproduce at a constant per-tick
// probability, paying an Energy cost to spawn an offspring at their own cell.
// Phase 3: the offspring's GeneticTraits are the parent's plus mutation.
class ReproductionSystem {
public:
    void update(entt::registry& registry, std::mt19937& rng, float dt,
                const ReproductionParams& reproParams, const GeneticsParams& geneticsParams);
};

} // namespace eco
