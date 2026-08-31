#pragma once

#include <entt/entt.hpp>

namespace eco {

// Agents whose Energy and Reproductive.readiness cross a threshold spawn offspring
// with mutated GeneticTraits.
class ReproductionSystem {
public:
    void update(entt::registry& registry);
};

} // namespace eco
