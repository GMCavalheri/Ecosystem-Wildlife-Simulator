#include <spdlog/spdlog.h>

#include "components/Components.h"
#include "core/Simulation.h"

int main() {
    spdlog::info("Ecosystem/Wildlife Simulator -- Phase 3: genetics + selection validation");

    eco::Simulation sim(64, 64);
    sim.seedPopulation(eco::kPreySpeciesId, 280);
    sim.seedPopulation(eco::kPredatorSpeciesId, 30);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 1000; // ~33 simulated time units: selection rise, then plateau

    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
    }

    const char* outputPath = "population_history.csv";
    sim.metrics().writeCsv(outputPath);

    spdlog::info("Ran {} ticks ({:.2f}s simulated). Wrote {}.", ticks,
                 sim.simulationTime(), outputPath);

    return 0;
}
