#pragma once

#include <cstdint>
#include <vector>

namespace eco {

// Phase 8: terrain types. Each biome sets a cell's vegetation carrying capacity and
// regrowth speed (see environment/Biomes.h); the default world is one uniform Plains.
enum class Biome : std::uint8_t { Plains, Forest, Desert, Wetland };

// Terrain is a flat array, not ECS entities, for cache locality on grid-wide scans.
struct Cell {
    float vegetationDensity = 1.0f;
    float waterAvailability = 1.0f;
    float temperature = 20.0f;
    // Phase 8: vegetation follows dV/dt = r * growthMultiplier * V * (1 - V/capacity).
    // The defaults (1, 1) reproduce the original uniform logistic exactly.
    float capacity = 1.0f;
    float growthMultiplier = 1.0f;
    Biome biome = Biome::Plains;
};

class Grid {
public:
    Grid(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    Cell& at(int x, int y);
    const Cell& at(int x, int y) const;

private:
    int width_;
    int height_;
    std::vector<Cell> cells_;
};

} // namespace eco
