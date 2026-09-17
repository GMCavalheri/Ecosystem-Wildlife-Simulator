#include <spdlog/spdlog.h>

#include "components/Components.h"
#include "core/Simulation.h"

int main() {
    spdlog::info("Ecosystem/Wildlife Simulator -- Phase 4: disease/overcrowding validation");

    // A smaller grid than earlier demos: 280 prey on 256 cells (vs. 4096) forces real
    // crowding, which is what lets the outbreak seeded below actually take off -- see
    // tools/plot_population.py's infected/immune panels and the Phase 4 write-up for
    // why the same disease parameters fizzle harmlessly on a sparser grid.
    eco::Simulation sim(16, 16);
    sim.seedPopulation(eco::kPreySpeciesId, 280);
    sim.seedPopulation(eco::kPredatorSpeciesId, 10);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int outbreakTick = 300; // t ~= 10: population already well into its boom
    constexpr int ticks = 900;        // ~30 simulated time units

    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
        if (i == outbreakTick) {
            sim.infectRandomPrey(20);
            spdlog::info("Outbreak seeded at t={:.2f}: infected 20 prey.", sim.simulationTime());
        }
    }

    const char* outputPath = "population_history.csv";
    sim.metrics().writeCsv(outputPath);

    spdlog::info("Ran {} ticks ({:.2f}s simulated). Wrote {}.", ticks,
                 sim.simulationTime(), outputPath);

    return 0;
}
