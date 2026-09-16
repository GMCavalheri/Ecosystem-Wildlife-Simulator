#include <spdlog/spdlog.h>

#include "components/Components.h"
#include "core/Simulation.h"

int main() {
    spdlog::info("Ecosystem/Wildlife Simulator -- predator resilience validation");

    eco::Simulation sim(64, 64);
    sim.seedPopulation(eco::kPreySpeciesId, 280);
    sim.seedPopulation(eco::kPredatorSpeciesId, 10);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 1800; // ~60 simulated time units: predators now grow to a
                                // healthy peak (~490) and sustain for 30+ time units,
                                // instead of the old near-instant collapse -- they
                                // still eventually decline here, but now tied to
                                // prey's own separate eventual collapse (they run out
                                // of food), not an arbitrary background death rate.

    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
    }

    const char* outputPath = "population_history.csv";
    sim.metrics().writeCsv(outputPath);

    spdlog::info("Ran {} ticks ({:.2f}s simulated). Wrote {}.", ticks,
                 sim.simulationTime(), outputPath);

    return 0;
}
