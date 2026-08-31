#pragma once

#include <entt/entt.hpp>

namespace eco {

// Removes entities at 0 energy, old age, or disease death.
class MortalitySystem {
public:
    void update(entt::registry& registry);
};

} // namespace eco
