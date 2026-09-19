#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <random>
#include <string>

#include <entt/entt.hpp>

#include "components/Components.h"
#include "core/EcosystemParams.h"
#include "core/MetricsRecorder.h"
#include "core/SimulationConfig.h"
#include "core/ThreadPool.h"
#include "environment/Grid.h"
#include "systems/DiseaseSystem.h"
#include "systems/EnvironmentSystem.h"
#include "systems/ForagingSystem.h"
#include "systems/MigrationSystem.h"
#include "systems/MortalitySystem.h"
#include "systems/PredationSystem.h"
#include "systems/ReproductionSystem.h"

namespace eco {

// Phase 7: cumulative wall-clock time (seconds) spent in each stage of tick(), for
// profiling. Cheap enough (a handful of steady_clock reads per tick) to leave on.
struct SystemTimings {
    double environment = 0.0;
    double foraging = 0.0;
    double predation = 0.0;
    double reproduction = 0.0;
    double disease = 0.0;
    double migration = 0.0;
    double mortality = 0.0;
    double metrics = 0.0;

    double total() const {
        return environment + foraging + predation + reproduction + disease + migration +
               mortality + metrics;
    }
};

// Owns the ECS registry and terrain grid, and drives the fixed system order each tick.
// Headless by design — rendering is an optional layer built on top of this.
class Simulation {
public:
    Simulation(int gridWidth, int gridHeight, PredatorParams predatorParams = {},
               VegetationParams vegParams = {}, ForagingParams foragingParams = {},
               ReproductionParams reproParams = {}, GeneticsParams geneticsParams = {},
               MigrationParams migrationParams = {}, DiseaseParams diseaseParams = {},
               unsigned rngSeed = 1234u);

    // Phase 8: builds the simulation a SimulationConfig describes (world size, seed and
    // every tunable). Populations and biomes are applied by buildSimulation()
    // (core/Persistence.h); this only sets up the empty world.
    explicit Simulation(const SimulationConfig& config);

    // The parameters this simulation is currently running with (including any changed
    // live through the mutable accessors below).
    SimulationConfig config() const;

    void tick(float dt);

    // Creates `count` entities of the given species. Prey get a random Position on the
    // grid, starting Energy (Phase 2), homogeneous starting GeneticTraits (Phase 3 --
    // variance emerges only from mutation), and a fully susceptible Health (Phase 4).
    // Predators get starting Energy too (the resilience fix) but remain
    // non-spatial/mean-field for now, so disease doesn't reach them yet.
    void seedPopulation(SpeciesId species, std::size_t count);

    // Phase 4: infects `count` randomly-chosen susceptible prey -- the explicit
    // "patient zero" seeding step for an outbreak, kept separate from population
    // seeding so tests/demos can control outbreak size independently of population
    // size.
    void infectRandomPrey(std::size_t count);

    // Phase 8: the competitor species (kCompetitorSpeciesId) forages and reproduces with
    // its own parameters; by default they are copies of the prey's, i.e. an identical
    // species (useful as a control: two identical species must split evenly).
    void setCompetitorParams(const ForagingParams& foraging, const ReproductionParams& reproduction) {
        competitorForagingParams_ = foraging;
        competitorReproParams_ = reproduction;
    }

    // --- Persistence support (core/Persistence.h) -------------------------------------
    // The RNG's full state as text (std::mt19937 stream form), and its inverse; plus
    // restoring the clock. A resumed run is only bit-identical if all three are restored.
    std::string rngState() const;
    void restoreRngState(const std::string& state);
    void restoreSimulationTime(float time) { simulationTime_ = time; }

    entt::registry& registry() { return registry_; }
    const entt::registry& registry() const { return registry_; }

    Grid& grid() { return grid_; }
    const Grid& grid() const { return grid_; }

    float simulationTime() const { return simulationTime_; }

    const SystemTimings& timings() const { return timings_; }

    // Phase 7: runs the parallel-safe systems (environment, migration) on `threads`
    // threads. The default is 1 (fully serial). Results are bit-identical for every
    // thread count -- parallel work is split into fixed-size chunks with pre-assigned
    // RNG seeds, never by thread.
    void setThreadCount(unsigned threads);

    MetricsRecorder& metrics() { return metricsRecorder_; }
    const MetricsRecorder& metrics() const { return metricsRecorder_; }

    // Mutable references for live tuning (Phase 6's viewer adjusts these from
    // keyboard input at runtime). Headless callers can ignore these entirely.
    VegetationParams& vegetationParams() { return vegParams_; }
    const VegetationParams& vegetationParams() const { return vegParams_; }
    DiseaseParams& diseaseParams() { return diseaseParams_; }
    const DiseaseParams& diseaseParams() const { return diseaseParams_; }
    MigrationParams& migrationParams() { return migrationParams_; }
    const MigrationParams& migrationParams() const { return migrationParams_; }

private:
    entt::registry registry_;
    Grid grid_;
    float simulationTime_ = 0.0f;
    SystemTimings timings_;
    unsigned initialSeed_ = 1234u;
    std::unique_ptr<ThreadPool> pool_; // null => serial
    std::mt19937 rng_;
    PredatorParams predatorParams_;
    VegetationParams vegParams_;
    ForagingParams foragingParams_;
    ReproductionParams reproParams_;
    ForagingParams competitorForagingParams_;
    ReproductionParams competitorReproParams_;
    GeneticsParams geneticsParams_;
    MigrationParams migrationParams_;
    DiseaseParams diseaseParams_;

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
