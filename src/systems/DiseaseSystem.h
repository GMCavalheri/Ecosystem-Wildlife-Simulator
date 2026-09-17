#pragma once

#include <random>

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

// SIR-style spread among prey, local rather than mean-field: a susceptible prey's
// infection risk scales with how many infected prey share its own cell, so crowded
// cells become disease hotspots -- a second, independent check on overcrowding
// alongside Phase 2's vegetation depletion. Predators remain non-spatial for now, so
// disease doesn't reach them yet.
class DiseaseSystem {
public:
    void update(entt::registry& registry, std::mt19937& rng, float dt,
                const DiseaseParams& params);
};

} // namespace eco
