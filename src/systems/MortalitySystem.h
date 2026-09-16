#pragma once

#include <entt/entt.hpp>

namespace eco {

// Both species now die of starvation (Energy depleted to zero) -- prey since Phase 2,
// predators since the resilience fix that replaced their flat background death rate
// with a real Energy economy (see PredatorParams). Age/disease death arrive once
// those systems are implemented.
class MortalitySystem {
public:
    void update(entt::registry& registry);
};

} // namespace eco
