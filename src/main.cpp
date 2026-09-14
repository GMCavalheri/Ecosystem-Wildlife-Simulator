#include <spdlog/spdlog.h>

#include "components/Components.h"
#include "core/Simulation.h"

int main() {
    spdlog::info("Ecosystem/Wildlife Simulator -- Phase 2: vegetation + foraging validation");

    eco::Simulation sim(64, 64);
    sim.seedPopulation(eco::kPreySpeciesId, 280);
    sim.seedPopulation(eco::kPredatorSpeciesId, 10);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 1200; // ~40 simulated time units: full boom/bust arc

    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
    }

    const char* outputPath = "population_history.csv";
    sim.metrics().writeCsv(outputPath);

    spdlog::info("Ran {} ticks ({:.2f}s simulated). Wrote {}.", ticks,
                 sim.simulationTime(), outputPath);

    return 0;
}
