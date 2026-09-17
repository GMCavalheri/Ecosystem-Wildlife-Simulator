#include "core/Simulation.h"

#include <algorithm>
#include <vector>

namespace eco {

Simulation::Simulation(int gridWidth, int gridHeight, PredatorParams predatorParams,
                        VegetationParams vegParams, ForagingParams foragingParams,
                        ReproductionParams reproParams, GeneticsParams geneticsParams,
                        MigrationParams migrationParams, DiseaseParams diseaseParams,
                        unsigned rngSeed)
    : grid_(gridWidth, gridHeight),
      rng_(rngSeed),
      predatorParams_(predatorParams),
      vegParams_(vegParams),
      foragingParams_(foragingParams),
      reproParams_(reproParams),
      geneticsParams_(geneticsParams),
      migrationParams_(migrationParams),
      diseaseParams_(diseaseParams) {}

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
            registry_.emplace<Health>(entity, 100.0f, false, 0.0f, false);
        } else if (species == kPredatorSpeciesId) {
            registry_.emplace<Energy>(entity, kInitialPredatorEnergy, 100.0f);
        }
    }
}

void Simulation::infectRandomPrey(std::size_t count) {
    std::vector<entt::entity> susceptible;
    auto view = registry_.view<Species, Health>();
    for (auto entity : view) {
        const auto& health = view.get<Health>(entity);
        if (view.get<Species>(entity).id == kPreySpeciesId && !health.infected && !health.immune) {
            susceptible.push_back(entity);
        }
    }

    std::shuffle(susceptible.begin(), susceptible.end(), rng_);
    const std::size_t toInfect = std::min(count, susceptible.size());
    for (std::size_t i = 0; i < toInfect; ++i) {
        auto& health = registry_.get<Health>(susceptible[i]);
        health.infected = true;
        health.infectionTimer = 0.0f;
    }
}

void Simulation::tick(float dt) {
    environmentSystem_.update(grid_, dt, simulationTime_, vegParams_);
    foragingSystem_.update(registry_, grid_, dt, foragingParams_);
    predationSystem_.update(registry_, rng_, dt, predatorParams_);
    reproductionSystem_.update(registry_, rng_, dt, reproParams_, geneticsParams_, predatorParams_);
    diseaseSystem_.update(registry_, rng_, dt, diseaseParams_);
    migrationSystem_.update(registry_, grid_, rng_, dt, migrationParams_);
    mortalitySystem_.update(registry_);

    simulationTime_ += dt;
    metricsRecorder_.snapshot(registry_, grid_, simulationTime_);
}

} // namespace eco
