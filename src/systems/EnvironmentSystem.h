#pragma once

namespace eco {

class Grid;

// Vegetation regrowth (logistic curve) and seasonal temperature/rainfall.
class EnvironmentSystem {
public:
    void update(Grid& grid, float dt);
};

} // namespace eco
