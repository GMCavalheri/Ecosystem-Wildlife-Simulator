#include "core/Simulation.h"

#include <algorithm>
#include <sstream>

#include "components/PreyGroup.h"
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
      competitorForagingParams_(foragingParams),
      competitorReproParams_(reproParams),
      geneticsParams_(geneticsParams),
      migrationParams_(migrationParams),
      diseaseParams_(diseaseParams) {
    initialSeed_ = rngSeed;
    // Create the prey group up front so it always exists (a lazily-created group would
    // make even a const save operation depend on -- or trigger -- its creation).
    (void)preyGroup(registry_);
}

Simulation::Simulation(const SimulationConfig& c)
    : Simulation(c.gridWidth, c.gridHeight, c.predator, c.vegetation, c.foraging,
                 c.reproduction, c.genetics, c.migration, c.disease, c.seed) {
    setCompetitorParams(c.competitorForaging, c.competitorReproduction);
}

SimulationConfig Simulation::config() const {
    SimulationConfig c;
    c.gridWidth = grid_.width();
    c.gridHeight = grid_.height();
    c.seed = initialSeed_;
    c.predator = predatorParams_;
    c.vegetation = vegParams_;
    c.foraging = foragingParams_;
    c.reproduction = reproParams_;
    c.competitorForaging = competitorForagingParams_;
    c.competitorReproduction = competitorReproParams_;
    c.genetics = geneticsParams_;
    c.migration = migrationParams_;
    c.disease = diseaseParams_;
    return c;
}

std::string Simulation::rngState() const {
    std::ostringstream out;
    out << rng_;
    return out.str();
}

void Simulation::restoreRngState(const std::string& state) {
    std::istringstream in(state);
    in >> rng_;
}

void Simulation::seedPopulation(SpeciesId species, std::size_t count) {
    std::uniform_int_distribution<int> xDist(0, grid_.width() - 1);
    std::uniform_int_distribution<int> yDist(0, grid_.height() - 1);

    for (std::size_t i = 0; i < count; ++i) {
        auto entity = registry_.create();
        registry_.emplace<Species>(entity, species);

        if (species == kPreySpeciesId || species == kCompetitorSpeciesId) {
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

void Simulation::setThreadCount(unsigned threads) {
    pool_ = threads > 1 ? std::make_unique<ThreadPool>(threads) : nullptr;
}

void Simulation::tick(float dt) {
    using Clock = std::chrono::steady_clock;
    auto stage = [](double& accumulator, auto&& run) {
        const auto start = Clock::now();
        run();
        accumulator += std::chrono::duration<double>(Clock::now() - start).count();
    };

    stage(timings_.environment,
          [&] { environmentSystem_.update(grid_, dt, simulationTime_, vegParams_, pool_.get()); });
    stage(timings_.foraging, [&] { foragingSystem_.update(registry_, grid_, dt, foragingParams_, competitorForagingParams_); });
    stage(timings_.predation,
          [&] { predationSystem_.update(registry_, rng_, dt, predatorParams_); });
    stage(timings_.reproduction, [&] {
        reproductionSystem_.update(registry_, rng_, dt, reproParams_, competitorReproParams_,
                                   geneticsParams_, predatorParams_);
    });
    stage(timings_.disease, [&] { diseaseSystem_.update(registry_, grid_, rng_, dt, diseaseParams_); });
    stage(timings_.migration,
          [&] { migrationSystem_.update(registry_, grid_, rng_, dt, migrationParams_, pool_.get()); });
    stage(timings_.mortality, [&] { mortalitySystem_.update(registry_); });

    simulationTime_ += dt;
    stage(timings_.metrics,
          [&] { metricsRecorder_.snapshot(registry_, grid_, simulationTime_); });
}

} // namespace eco
