#pragma once

#include <entt/entt.hpp>

#include "components/Components.h"

namespace eco {

// Phase 7: herbivores are exactly the entities that carry all five of Position, Energy,
// GeneticTraits, Health and Species (seedPopulation and ReproductionSystem attach them
// together); predators only have Energy and Species. Phase 8: "prey" in this group means
// every herbivore -- the original prey and the competitor species alike -- and Species is
// part of the group so a loop can choose each animal's species-specific parameters
// without a sparse lookup.
//
// Registering these as an *owning group* makes EnTT keep the five pools physically
// aligned -- same entities, same order -- so a loop over prey is a straight walk over
// contiguous arrays instead of a sparse-set lookup per component. Measured 3.2x faster
// for ForagingSystem at ~44k entities, versus a plain view (whose pools drift out of
// order as animals are born and die).
//
// An owning group takes exclusive ownership of its pools, so nothing else may own
// Position/Energy/GeneticTraits/Health/Species in another group; plain views remain fine.
inline auto preyGroup(entt::registry& registry) {
    return registry.group<Position, Energy, GeneticTraits, Health, Species>();
}

// For const callers (e.g. MetricsRecorder): the group already exists once the
// simulation has ticked; falsy if it hasn't been created yet.
inline auto preyGroup(const entt::registry& registry) {
    return registry.group_if_exists<Position, Energy, GeneticTraits, Health, Species>();
}

} // namespace eco
