#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "core/Persistence.h"

namespace {

void printUsage() {
    std::cout <<
        "Usage: ecosystem_sim [options]\n"
        "  --config FILE    start a fresh run from a scenario file (partial JSON is fine:\n"
        "                   omitted keys keep their defaults; see data/)\n"
        "  --load FILE      resume a saved run instead (continues bit-identically)\n"
        "  --save FILE      write the final state to FILE\n"
        "  --ticks N        ticks to run (default 900, ~30 simulated time units)\n"
        "  --csv FILE       write the population history (default population_history.csv)\n"
        "  --threads N      threads for the parallel-safe systems (default 1; results\n"
        "                   are identical for any N)\n"
        "  --dump-config    print the default configuration as JSON and exit\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string configPath, loadPath, savePath, csvPath = "population_history.csv";
    int ticks = 900;
    unsigned threads = 1;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "missing value for " << arg << "\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (arg == "--config") configPath = value();
        else if (arg == "--load") loadPath = value();
        else if (arg == "--save") savePath = value();
        else if (arg == "--csv") csvPath = value();
        else if (arg == "--ticks") ticks = std::atoi(value().c_str());
        else if (arg == "--threads") threads = static_cast<unsigned>(std::atoi(value().c_str()));
        else if (arg == "--dump-config") {
            std::cout << eco::toReadableJson(eco::SimulationConfig{}).dump(2) << "\n";
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else {
            std::cerr << "unknown option: " << arg << "\n";
            printUsage();
            return 2;
        }
    }
    if (!configPath.empty() && !loadPath.empty()) {
        std::cerr << "--config starts a fresh run and --load resumes one; pick one\n";
        return 2;
    }

    try {
        eco::Simulation sim = !loadPath.empty()   ? eco::loadStateFromFile(loadPath)
                              : !configPath.empty() ? eco::buildSimulation(eco::loadConfigFile(configPath))
                                                    : eco::buildSimulation(eco::SimulationConfig{});
        sim.setThreadCount(threads);
        spdlog::info("{} {} ({}x{} grid, t={:.2f}).", loadPath.empty() ? "Starting" : "Resumed",
                     loadPath.empty() ? (configPath.empty() ? "default scenario" : configPath)
                                      : loadPath,
                     sim.grid().width(), sim.grid().height(), sim.simulationTime());

        for (int i = 0; i < ticks; ++i) {
            sim.tick(1.0f / 30.0f);
        }

        sim.metrics().writeCsv(csvPath);
        const auto& last = sim.metrics().history().back();
        spdlog::info("Ran {} ticks (t={:.2f}): prey={} competitors={} predators={}. Wrote {}.",
                     ticks, sim.simulationTime(), last.preyCount, last.competitorCount,
                     last.predatorCount, csvPath);

        if (!savePath.empty()) {
            eco::saveStateToFile(sim, savePath);
            spdlog::info("Saved state to {}.", savePath);
        }
    } catch (const std::exception& e) {
        spdlog::error("{}", e.what());
        return 1;
    }
    return 0;
}
