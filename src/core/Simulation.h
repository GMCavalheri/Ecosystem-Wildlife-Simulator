#pragma once

#include <entt/entt.hpp>

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
    Simulation(int gridWidth, int gridHeight);

    void tick(float dt);

    entt::registry& registry() { return registry_; }
    const entt::registry& registry() const { return registry_; }

    Grid& grid() { return grid_; }
    const Grid& grid() const { return grid_; }

    float simulationTime() const { return simulationTime_; }

private:
    entt::registry registry_;
    Grid grid_;
    float simulationTime_ = 0.0f;

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
