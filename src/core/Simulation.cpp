#include "core/Simulation.h"

namespace eco {

Simulation::Simulation(int gridWidth, int gridHeight, LotkaVolterraParams lvParams,
                        unsigned rngSeed)
    : grid_(gridWidth, gridHeight), rng_(rngSeed), lvParams_(lvParams) {}

void Simulation::seedPopulation(SpeciesId species, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        auto entity = registry_.create();
        registry_.emplace<Species>(entity, species);
    }
}

void Simulation::tick(float dt) {
    environmentSystem_.update(grid_, dt);
    foragingSystem_.update(registry_, grid_);
    predationSystem_.update(registry_, rng_, dt, lvParams_);
    reproductionSystem_.update(registry_, rng_, dt, lvParams_);
    diseaseSystem_.update(registry_, grid_);
    migrationSystem_.update(registry_, grid_);
    mortalitySystem_.update(registry_, rng_, dt, lvParams_);

    simulationTime_ += dt;
    metricsRecorder_.snapshot(registry_, simulationTime_);
}

} // namespace eco
