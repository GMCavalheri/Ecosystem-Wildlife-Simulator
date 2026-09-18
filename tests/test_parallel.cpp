#include <atomic>
#include <cstdint>
#include <numeric>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "components/Components.h"
#include "core/Simulation.h"
#include "core/ThreadPool.h"

TEST_CASE("ThreadPool runs every index exactly once, job after job", "[parallel]") {
    for (unsigned threads : {1u, 2u, 5u, 12u}) {
        eco::ThreadPool pool(threads);
        REQUIRE(pool.threadCount() == threads);

        for (int job = 0; job < 300; ++job) {
            const std::size_t count = 1 + static_cast<std::size_t>(job % 37);
            std::vector<std::atomic<int>> hits(count);
            pool.parallelFor(count, [&](std::size_t i) { hits[i].fetch_add(1); });
            for (std::size_t i = 0; i < count; ++i) {
                REQUIRE(hits[i].load() == 1);
            }
        }
    }
}

TEST_CASE("ThreadPool handles empty and single-item jobs and clean shutdown", "[parallel]") {
    eco::ThreadPool pool(6);
    int calls = 0;
    pool.parallelFor(0, [&](std::size_t) { ++calls; });
    REQUIRE(calls == 0);
    pool.parallelFor(1, [&](std::size_t) { ++calls; });
    REQUIRE(calls == 1);
}

namespace {

struct RunSummary {
    std::vector<std::size_t> prey;
    std::vector<std::size_t> predator;
    std::vector<float> vegetation;
    std::vector<float> speed;
    std::uint64_t positionChecksum = 0;
};

RunSummary runSimulation(unsigned threads) {
    // 12,000 prey => 3 migration chunks of 4,096; 128 rows => 8 environment blocks, so
    // the parallel paths genuinely split work rather than degenerating to one chunk.
    eco::Simulation sim(128, 128, {}, {}, {}, {}, {}, {}, {}, /*rngSeed=*/2024u);
    sim.setThreadCount(threads);
    sim.seedPopulation(eco::kPreySpeciesId, 12000);
    sim.seedPopulation(eco::kPredatorSpeciesId, 12);
    sim.infectRandomPrey(120);

    RunSummary summary;
    for (int i = 0; i < 60; ++i) {
        sim.tick(1.0f / 30.0f);
        const auto& s = sim.metrics().history().back();
        summary.prey.push_back(s.preyCount);
        summary.predator.push_back(s.predatorCount);
        summary.vegetation.push_back(s.avgVegetation);
        summary.speed.push_back(s.avgPreySpeed);
    }
    // Order-independent fingerprint of where every prey ended up.
    for (auto [entity, position] : sim.registry().view<const eco::Position>().each()) {
        summary.positionChecksum += static_cast<std::uint64_t>(position.cellX) * 1000003ULL +
                                    static_cast<std::uint64_t>(position.cellY) * 7919ULL;
    }
    return summary;
}

} // namespace

// The whole point of splitting work into fixed-size chunks with pre-assigned RNG seeds
// (instead of per-thread work and per-thread RNGs) is that adding threads must never
// change the simulation. Same seed, 1 vs 3 vs 8 threads: exactly equal, not "close".
TEST_CASE("Simulation results are bit-identical regardless of thread count", "[parallel]") {
    const RunSummary serial = runSimulation(1);
    REQUIRE(serial.prey.back() > 4096); // still large enough to exercise multiple chunks

    for (unsigned threads : {3u, 8u}) {
        const RunSummary parallel = runSimulation(threads);
        REQUIRE(parallel.prey == serial.prey);
        REQUIRE(parallel.predator == serial.predator);
        REQUIRE(parallel.vegetation == serial.vegetation);
        REQUIRE(parallel.speed == serial.speed);
        REQUIRE(parallel.positionChecksum == serial.positionChecksum);
    }
}
