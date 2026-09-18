#pragma once

#include <entt/entt.hpp>

#include "components/Components.h"

namespace eco {

// Phase 7: prey are exactly the entities that carry all four of Position, Energy,
// GeneticTraits and Health (seedPopulation and ReproductionSystem attach them
// together); predators only have Energy. Registering these as an *owning group* makes
// EnTT keep the four pools physically aligned -- same entities, same order -- so a loop
// over prey is a straight walk over contiguous arrays instead of a sparse-set lookup
// per component. Measured 3.2x faster for ForagingSystem at ~44k entities, versus a
// plain view (whose pools drift out of order as animals are born and die).
//
// An owning group takes exclusive ownership of its pools, so nothing else may own
// Position/Energy/GeneticTraits/Health in another group; plain views remain fine.
inline auto preyGroup(entt::registry& registry) {
    return registry.group<Position, Energy, GeneticTraits, Health>();
}

// For const callers (e.g. MetricsRecorder): the group already exists once the
// simulation has ticked; falsy if it hasn't been created yet.
inline auto preyGroup(const entt::registry& registry) {
    return registry.group_if_exists<Position, Energy, GeneticTraits, Health>();
}

} // namespace eco
