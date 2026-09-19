#pragma once

#include <cstddef>

#include "core/EcosystemParams.h"

namespace eco {

// Phase 8: how many of each animal a fresh scenario starts with.
struct InitialPopulation {
    std::size_t prey = 280;
    std::size_t competitors = 0;
    std::size_t predators = 10;
    std::size_t infected = 0; // prey infected at t=0 ("patient zero")
};

// Phase 8: optional biome map for a fresh scenario (see environment/Biomes.h).
struct BiomeConfig {
    bool enabled = false;
    unsigned seed = 1;
    float latticeSpacing = 12.0f;
};

// Everything that defines a scenario: world size, RNG seed, every tunable, the starting
// populations, and the biome map. Loadable from JSON (core/Persistence.h) so tunables
// can be edited without recompiling; a save file embeds one to record the parameters a
// run was using.
struct SimulationConfig {
    int gridWidth = 64;
    int gridHeight = 64;
    unsigned seed = 1234;

    PredatorParams predator;
    VegetationParams vegetation;
    ForagingParams foraging;
    ReproductionParams reproduction;
    ForagingParams competitorForaging;       // Phase 8's second herbivore species
    ReproductionParams competitorReproduction;
    GeneticsParams genetics;
    MigrationParams migration;
    DiseaseParams disease;

    InitialPopulation initial;
    BiomeConfig biomes;
};

} // namespace eco
