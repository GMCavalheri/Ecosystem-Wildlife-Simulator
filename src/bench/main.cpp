#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "components/Components.h"
#include "core/Simulation.h"
#include "core/ThreadPool.h"

#include <vector>

namespace {

struct SweepResult {
    std::size_t prey = 0;
    std::size_t predators = 0;
    float speed = 0.0f;
};

// One independent replicate: the standard scenario with its own seed. Shares nothing
// with any other replicate, which is why running many at once scales almost linearly.
SweepResult runReplicate(unsigned seed, int ticks) {
    eco::Simulation sim(64, 64, {}, {}, {}, {}, {}, {}, {}, seed);
    sim.seedPopulation(eco::kPreySpeciesId, 280);
    sim.seedPopulation(eco::kPredatorSpeciesId, 30);
    for (int i = 0; i < ticks; ++i) {
        sim.tick(1.0f / 30.0f);
    }
    const auto& s = sim.metrics().history().back();
    return {s.preyCount, s.predatorCount, s.avgPreySpeed};
}

// Usage: ecosystem_bench sweep [replicates] [ticks] [threads]
int runSweep(int argc, char** argv) {
    const int replicates = argc > 2 ? std::atoi(argv[2]) : 48;
    const int ticks = argc > 3 ? std::atoi(argv[3]) : 600;
    const unsigned threads = argc > 4 ? static_cast<unsigned>(std::atoi(argv[4])) : 12;

    using Clock = std::chrono::steady_clock;
    std::vector<SweepResult> serial(replicates), parallel(replicates);

    auto t0 = Clock::now();
    for (int i = 0; i < replicates; ++i) {
        serial[i] = runReplicate(1000 + i, ticks);
    }
    const double serialSeconds = std::chrono::duration<double>(Clock::now() - t0).count();

    eco::ThreadPool pool(threads);
    t0 = Clock::now();
    pool.parallelFor(replicates, [&](std::size_t i) {
        parallel[i] = runReplicate(1000 + static_cast<unsigned>(i), ticks);
    });
    const double parallelSeconds = std::chrono::duration<double>(Clock::now() - t0).count();

    bool identical = true;
    for (int i = 0; i < replicates; ++i) {
        identical = identical && serial[i].prey == parallel[i].prey &&
                    serial[i].predators == parallel[i].predators && serial[i].speed == parallel[i].speed;
    }
    std::printf("sweep: %d replicates x %d ticks\n", replicates, ticks);
    std::printf("  serial   (1 thread):   %7.3f s\n", serialSeconds);
    std::printf("  parallel (%u threads): %7.3f s   speedup %.2fx\n", threads, parallelSeconds,
                serialSeconds / parallelSeconds);
    std::printf("  results identical to serial run: %s\n", identical ? "yes" : "NO");
    return identical ? 0 : 1;
}

} // namespace

// Phase 7 benchmark: seeds a large population on a large grid, runs a fixed number of
// ticks, and reports where the time went. Usage: ecosystem_bench [prey] [gridSide] [ticks] [threads]
int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "sweep") {
        return runSweep(argc, argv);
    }
    const std::size_t prey = argc > 1 ? std::strtoul(argv[1], nullptr, 10) : 20000;
    const int side = argc > 2 ? std::atoi(argv[2]) : 256;
    const int ticks = argc > 3 ? std::atoi(argv[3]) : 100;
    const unsigned threads = argc > 4 ? static_cast<unsigned>(std::atoi(argv[4])) : 1;

    eco::Simulation sim(side, side);
    sim.setThreadCount(threads);
    sim.seedPopulation(eco::kPreySpeciesId, prey);
    sim.seedPopulation(eco::kPredatorSpeciesId, prey / 1000);
    sim.infectRandomPrey(prey / 100);

    constexpr float dt = 1.0f / 30.0f;
    const auto start = std::chrono::steady_clock::now();
    std::size_t entitySum = 0;
    for (int i = 0; i < ticks; ++i) {
        sim.tick(dt);
        const auto& s = sim.metrics().history().back();
        entitySum += s.preyCount + s.predatorCount;
    }
    const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    const auto& t = sim.timings();
    const auto& last = sim.metrics().history().back();
    std::printf("threads=%u prey0=%zu grid=%dx%d ticks=%d  final: prey=%zu pred=%zu  avg entities/tick=%zu\n",
                threads, prey, side, side, ticks, last.preyCount, last.predatorCount, entitySum / ticks);
    std::printf("wall %.3fs  =>  %.2f ms/tick\n", wall, wall * 1000.0 / ticks);
    auto row = [&](const char* name, double v) {
        std::printf("  %-13s %8.3f ms/tick  %5.1f%%\n", name, v * 1000.0 / ticks, 100.0 * v / t.total());
    };
    row("environment", t.environment);
    row("foraging", t.foraging);
    row("predation", t.predation);
    row("reproduction", t.reproduction);
    row("disease", t.disease);
    row("migration", t.migration);
    row("mortality", t.mortality);
    row("metrics", t.metrics);
    return 0;
}
