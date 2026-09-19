#pragma once

#include "environment/Grid.h"

namespace eco {

// Phase 8: what a biome does to the vegetation growing in it.
struct BiomeProperties {
    float capacity;         // K: standing vegetation the cell can support
    float growthMultiplier; // scales the regrowth rate
};

// Desert: sparse and slow. Plains: moderate. Forest: lots of standing biomass but slow
// to regrow (a grazed forest recovers slowly). Wetland: lush and fast.
constexpr BiomeProperties biomeProperties(Biome biome) {
    switch (biome) {
    case Biome::Desert:  return {0.25f, 0.5f};
    case Biome::Plains:  return {0.70f, 1.0f};
    case Biome::Forest:  return {1.00f, 0.7f};
    case Biome::Wetland: return {1.00f, 1.4f};
    }
    return {1.0f, 1.0f};
}

// Fills the grid with a smooth, deterministic biome map: a seeded value-noise "moisture"
// field (random values on a coarse lattice, smoothly interpolated between them) is
// ranked and split by quantile -- driest 15% Desert, then 35% Plains, 35% Forest,
// wettest 15% Wetland -- so every biome is guaranteed to exist whatever the seed.
// Each cell gets its biome's capacity/growth multiplier, its moisture as
// waterAvailability, and starts with vegetation at carrying capacity.
// `latticeSpacing` is the size of a biome patch in cells (larger = bigger regions).
void generateBiomes(Grid& grid, unsigned seed, float latticeSpacing = 12.0f);

} // namespace eco
