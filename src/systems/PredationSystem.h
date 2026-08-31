#pragma once

#include <entt/entt.hpp>

namespace eco {

class Grid;

// Kill probability is a function of predator hunger, prey density, and relative
// GeneticTraits.speed — not a fixed rate.
class PredationSystem {
public:
    void update(entt::registry& registry, Grid& grid);
};

} // namespace eco
