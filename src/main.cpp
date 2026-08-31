#include <spdlog/spdlog.h>

#include "core/Simulation.h"

int main() {
    spdlog::info("Ecosystem/Wildlife Simulator -- headless bootstrap");

    eco::Simulation sim(64, 64);

    constexpr float dt = 1.0f / 30.0f;
    constexpr int ticks = 10;

    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
    }

    spdlog::info("Ran {} ticks ({:.2f}s simulated).", ticks, sim.simulationTime());

    return 0;
}
