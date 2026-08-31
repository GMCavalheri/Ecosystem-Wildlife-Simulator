#pragma once

#include <entt/entt.hpp>

namespace eco {

class Grid;

// SIR-style spread; infection probability scales with local population density
// (contact rate), producing self-correcting overcrowding.
class DiseaseSystem {
public:
    void update(entt::registry& registry, Grid& grid);
};

} // namespace eco
