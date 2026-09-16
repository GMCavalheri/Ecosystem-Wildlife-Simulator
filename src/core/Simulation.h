#pragma once

#include <cstddef>
#include <random>

#include <entt/entt.hpp>

#include "components/Components.h"
#include "core/EcosystemParams.h"
#include "core/MetricsRecorder.h"
#include "environment/Grid.h"
#include "systems/DiseaseSystem.h"
#include "systems/EnvironmentSystem.h"
#include "systems/ForagingSystem.h"
#include "systems/MigrationSystem.h"
#include "systems/MortalitySystem.h"
#include "systems/PredationSystem.h"
#include "systems/ReproductionSystem.h"

namespace eco {

// Owns the ECS registry and terrain grid, and drives the fixed system order each tick.
// Headless by design — rendering is an optional layer built on top of this.
class Simulation {
public:
    Simulation(int gridWidth, int gridHeight, PredatorParams predatorParams = {},
               VegetationParams vegParams = {}, ForagingParams foragingParams = {},
               ReproductionParams reproParams = {}, GeneticsParams geneticsParams = {},
               MigrationParams migrationParams = {}, unsigned rngSeed = 1234u);

    void tick(float dt);

    // Creates `count` entities of the given species. Prey get a random Position on the
    // grid, starting Energy (Phase 2), and homogeneous starting GeneticTraits (Phase 3
    // -- variance emerges only from mutation). Predators get starting Energy too (the
    // resilience fix) but remain non-spatial/mean-field for now.
    void seedPopulation(SpeciesId species, std::size_t count);

    entt::registry& registry() { return registry_; }
    const entt::registry& registry() const { return registry_; }

    Grid& grid() { return grid_; }
    const Grid& grid() const { return grid_; }

    float simulationTime() const { return simulationTime_; }

    MetricsRecorder& metrics() { return metricsRecorder_; }
    const MetricsRecorder& metrics() const { return metricsRecorder_; }

private:
    entt::registry registry_;
    Grid grid_;
    float simulationTime_ = 0.0f;
    std::mt19937 rng_;
    PredatorParams predatorParams_;
    VegetationParams vegParams_;
    ForagingParams foragingParams_;
    ReproductionParams reproParams_;
    GeneticsParams geneticsParams_;
    MigrationParams migrationParams_;

    EnvironmentSystem environmentSystem_;
    ForagingSystem foragingSystem_;
    PredationSystem predationSystem_;
    ReproductionSystem reproductionSystem_;
    DiseaseSystem diseaseSystem_;
    MigrationSystem migrationSystem_;
    MortalitySystem mortalitySystem_;
    MetricsRecorder metricsRecorder_;
};

} // namespace eco
