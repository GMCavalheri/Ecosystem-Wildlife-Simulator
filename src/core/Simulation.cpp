#include "core/Simulation.h"

namespace eco {

Simulation::Simulation(int gridWidth, int gridHeight, LotkaVolterraParams lvParams,
                        VegetationParams vegParams, ForagingParams foragingParams,
                        ReproductionParams reproParams, GeneticsParams geneticsParams,
                        unsigned rngSeed)
    : grid_(gridWidth, gridHeight),
      rng_(rngSeed),
      lvParams_(lvParams),
      vegParams_(vegParams),
      foragingParams_(foragingParams),
      reproParams_(reproParams),
      geneticsParams_(geneticsParams) {}

void Simulation::seedPopulation(SpeciesId species, std::size_t count) {
    std::uniform_int_distribution<int> xDist(0, grid_.width() - 1);
    std::uniform_int_distribution<int> yDist(0, grid_.height() - 1);

    for (std::size_t i = 0; i < count; ++i) {
        auto entity = registry_.create();
        registry_.emplace<Species>(entity, species);

        if (species == kPreySpeciesId) {
            registry_.emplace<Position>(entity, xDist(rng_), yDist(rng_));
            registry_.emplace<Energy>(entity, kInitialPreyEnergy, 100.0f);
            registry_.emplace<GeneticTraits>(entity, 1.0f, 1.0f, 1.0f);
        }
    }
}

void Simulation::tick(float dt) {
    environmentSystem_.update(grid_, dt, simulationTime_, vegParams_);
    foragingSystem_.update(registry_, grid_, dt, foragingParams_);
    predationSystem_.update(registry_, rng_, dt, lvParams_);
    reproductionSystem_.update(registry_, rng_, dt, reproParams_, geneticsParams_);
    diseaseSystem_.update(registry_, grid_);
    migrationSystem_.update(registry_, grid_);
    mortalitySystem_.update(registry_, rng_, dt, lvParams_);

    simulationTime_ += dt;
    metricsRecorder_.snapshot(registry_, grid_, simulationTime_);
}

} // namespace eco
