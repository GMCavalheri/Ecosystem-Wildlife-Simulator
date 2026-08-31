#include "core/Simulation.h"

namespace eco {

Simulation::Simulation(int gridWidth, int gridHeight) : grid_(gridWidth, gridHeight) {}

void Simulation::tick(float dt) {
    environmentSystem_.update(grid_, dt);
    foragingSystem_.update(registry_, grid_);
    predationSystem_.update(registry_, grid_);
    reproductionSystem_.update(registry_);
    diseaseSystem_.update(registry_, grid_);
    migrationSystem_.update(registry_, grid_);
    mortalitySystem_.update(registry_);
    metricsRecorder_.snapshot(registry_);

    simulationTime_ += dt;
}

} // namespace eco
