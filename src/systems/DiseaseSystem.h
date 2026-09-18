#pragma once

#include <random>
#include <vector>

#include <entt/entt.hpp>

#include "core/EcosystemParams.h"

namespace eco {

class Grid;

// SIR-style spread among prey, local rather than mean-field: a susceptible prey's
// infection risk scales with how many infected prey share its own cell, so crowded
// cells become disease hotspots -- a second, independent check on overcrowding
// alongside Phase 2's vegetation depletion. Predators remain non-spatial for now, so
// disease doesn't reach them yet.
//
// Phase 7: per-cell infected counts live in a flat, reused array indexed by cell
// (replacing a per-tick unordered_map of per-cell vectors), and the tick returns
// immediately when nobody is infected.
class DiseaseSystem {
public:
    void update(entt::registry& registry, const Grid& grid, std::mt19937& rng, float dt,
                const DiseaseParams& params);

private:
    std::vector<int> infectedPerCell_;
    std::vector<entt::entity> toRecover_;
    std::vector<entt::entity> toKill_;
};

} // namespace eco
