#include <cstddef>
#include <filesystem>
#include <functional>
#include <random>
#include <fstream>
#include <stdexcept>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <nlohmann/json.hpp>

#include "components/Components.h"
#include "core/Persistence.h"

namespace {

// A scenario that exercises everything a save has to carry: biomes, a latitude climate,
// both herbivore species (with different parameters), predators, and an outbreak.
eco::SimulationConfig richScenario() {
    eco::SimulationConfig config;
    config.gridWidth = 40;
    config.gridHeight = 40;
    config.seed = 99;
    config.vegetation.latitudeGradient = 20.0f;
    config.competitorForaging.metabolicRate = 8.0f;
    config.competitorReproduction.attemptRate = 0.35f;
    config.initial = {600, 300, 15, 30};
    config.biomes.enabled = true;
    config.biomes.seed = 5;
    return config;
}

eco::Simulation runFor(eco::Simulation sim, int ticks) {
    for (int i = 0; i < ticks; ++i) sim.tick(1.0f / 30.0f);
    return sim;
}

} // namespace

TEST_CASE("Config JSON round-trips, and partial files keep defaults", "[persistence]") {
    const eco::SimulationConfig original = richScenario();
    const auto json = eco::toJson(original);
    const eco::SimulationConfig restored = eco::configFromJson(json);
    REQUIRE(eco::toJson(restored) == json);
    REQUIRE(restored.competitorForaging.metabolicRate == 8.0f);
    REQUIRE(restored.biomes.enabled);

    // Only the keys a scenario changes need to appear; everything else is the default.
    const auto partial = nlohmann::json::parse(
        R"({"vegetation": {"regrowthRate": 0.9}, "initial": {"prey": 50}})");
    const eco::SimulationConfig config = eco::configFromJson(partial);
    const eco::SimulationConfig defaults;
    REQUIRE(config.vegetation.regrowthRate == 0.9f);
    REQUIRE(config.vegetation.seasonalPeriod == defaults.vegetation.seasonalPeriod);
    REQUIRE(config.initial.prey == 50);
    REQUIRE(config.initial.predators == defaults.initial.predators);
    REQUIRE(config.gridWidth == defaults.gridWidth);
}

TEST_CASE("The human-readable config dump is tidy and still round-trips exactly",
          "[persistence]") {
    const eco::SimulationConfig original = richScenario();
    const nlohmann::json readable = eco::toReadableJson(original);
    REQUIRE(readable["foraging"]["maxIntakeRate"] == 0.12); // not 0.11999999731779099
    REQUIRE(readable["vegetation"]["latitudeGradient"] == 20.0);
    // Reading it back reproduces every float bit-for-bit.
    REQUIRE(eco::toJson(eco::configFromJson(readable)) == eco::toJson(original));
    REQUIRE(eco::toJson(eco::configFromJson(eco::toReadableJson(eco::SimulationConfig{}))) ==
            eco::toJson(eco::SimulationConfig{}));
}

TEST_CASE("Config loading rejects typos and wrong types instead of ignoring them",
          "[persistence]") {
    using Catch::Matchers::ContainsSubstring;
    REQUIRE_THROWS_WITH(eco::configFromJson(nlohmann::json::parse(R"({"vegetation": {"regrowthrate": 1}})")),
                        ContainsSubstring("vegetation.regrowthrate"));
    REQUIRE_THROWS_WITH(eco::configFromJson(nlohmann::json::parse(R"({"notASection": 1})")),
                        ContainsSubstring("notASection"));
    REQUIRE_THROWS_AS(eco::configFromJson(nlohmann::json::parse(R"({"gridWidth": "wide"})")),
                      std::runtime_error);
}

TEST_CASE("A saved state reloads to an identical state, animal order included",
          "[persistence]") {
    // 120 ticks of births, deaths, disease and migration scramble the internal order of
    // the animals, so this is not just "a fresh world saved and loaded".
    eco::Simulation original = runFor(eco::buildSimulation(richScenario()), 120);
    REQUIRE(original.metrics().history().back().competitorCount > 0);
    REQUIRE(original.metrics().history().back().infectedPreyCount > 0);

    const nlohmann::json saved = eco::saveState(original);
    eco::Simulation restored = eco::loadState(saved);

    // Saving the restored simulation must reproduce the file exactly: the clock, RNG
    // state, every grid cell, and every animal in the same order.
    REQUIRE(eco::saveState(restored) == saved);
}

// The strongest form of "it works": a run resumed from a save is indistinguishable from
// one that never stopped. Same RNG state + same animal order => the same random draws
// land on the same animals for every subsequent tick.
TEST_CASE("A resumed simulation continues bit-identically to one that never stopped",
          "[persistence]") {
    eco::Simulation continuous = runFor(eco::buildSimulation(richScenario()), 120);
    eco::Simulation resumed = eco::loadState(eco::saveState(continuous));

    constexpr int extraTicks = 100;
    const std::size_t historyStart = continuous.metrics().history().size();
    for (int i = 0; i < extraTicks; ++i) {
        continuous.tick(1.0f / 30.0f);
        resumed.tick(1.0f / 30.0f);
    }

    // Every tick's population statistics match exactly...
    const auto& a = continuous.metrics().history();
    const auto& b = resumed.metrics().history();
    REQUIRE(a.size() == historyStart + extraTicks);
    REQUIRE(b.size() == 1 + extraTicks); // the resume point, then the new ticks
    for (int i = 0; i < extraTicks; ++i) {
        const auto& x = a[historyStart + i];
        const auto& y = b[1 + i];
        REQUIRE(x.time == y.time);
        REQUIRE(x.preyCount == y.preyCount);
        REQUIRE(x.competitorCount == y.competitorCount);
        REQUIRE(x.predatorCount == y.predatorCount);
        REQUIRE(x.infectedPreyCount == y.infectedPreyCount);
        REQUIRE(x.avgVegetation == y.avgVegetation);
        REQUIRE(x.avgPreySpeed == y.avgPreySpeed);
    }
    // ...and so does the complete final state.
    REQUIRE(eco::saveState(resumed) == eco::saveState(continuous));
}

TEST_CASE("Saving to and loading from a file works", "[persistence]") {
    eco::Simulation original = runFor(eco::buildSimulation(richScenario()), 40);
    const auto path = std::filesystem::temp_directory_path() / "ecosystem_test_save.json";
    eco::saveStateToFile(original, path);
    eco::Simulation restored = eco::loadStateFromFile(path);
    std::filesystem::remove(path);
    REQUIRE(eco::saveState(restored) == eco::saveState(original));
}

TEST_CASE("Loading rejects corrupt or incompatible saves with clear errors", "[persistence]") {
    using Catch::Matchers::ContainsSubstring;
    const nlohmann::json good = eco::saveState(eco::buildSimulation(richScenario()));

    auto mutated = [&](auto edit) {
        nlohmann::json j = good;
        edit(j);
        return j;
    };

    REQUIRE_THROWS_WITH(eco::loadState(mutated([](auto& j) { j["version"] = 999; })),
                        ContainsSubstring("unsupported version 999"));
    REQUIRE_THROWS_WITH(eco::loadState(mutated([](auto& j) { j.erase("rng"); })),
                        ContainsSubstring("missing field 'rng'"));
    REQUIRE_THROWS_WITH(eco::loadState(mutated([](auto& j) { j["herbivores"][0][1] = 9999; })),
                        ContainsSubstring("outside the grid"));
    REQUIRE_THROWS_WITH(eco::loadState(mutated([](auto& j) { j["herbivores"][0][0] = 77; })),
                        ContainsSubstring("unknown herbivore species"));
    REQUIRE_THROWS_WITH(eco::loadState(mutated([](auto& j) { j["herbivores"][0].push_back(1); })),
                        ContainsSubstring("malformed herbivore row"));
    REQUIRE_THROWS_WITH(eco::loadState(mutated([](auto& j) { j["grid"]["cells"].erase(0); })),
                        ContainsSubstring("wrong number of grid cells"));
    REQUIRE_THROWS_WITH(eco::loadState(mutated([](auto& j) { j["grid"]["width"] = 41; })),
                        ContainsSubstring("does not match the config"));
}

// The shipped scenario files are documentation people copy from, so they must not rot:
// every one has to still parse (no unknown keys after a rename) and actually run.
TEST_CASE("Every shipped data/*.json scenario loads and runs", "[persistence]") {
    int checked = 0;
    for (const auto& entry : std::filesystem::directory_iterator(ECOSYSTEM_DATA_DIR)) {
        if (entry.path().extension() != ".json") continue;
        INFO(entry.path().filename().string());
        eco::Simulation sim = eco::buildSimulation(eco::loadConfigFile(entry.path()));
        for (int i = 0; i < 20; ++i) sim.tick(1.0f / 30.0f);
        REQUIRE(sim.metrics().history().size() == 20);
        ++checked;
    }
    REQUIRE(checked >= 4);
}

TEST_CASE("data/default_config.json is exactly the built-in defaults", "[persistence]") {
    const auto path = std::filesystem::path(ECOSYSTEM_DATA_DIR) / "default_config.json";
    REQUIRE(eco::toJson(eco::loadConfigFile(path)) == eco::toJson(eco::SimulationConfig{}));
}

TEST_CASE("Absurd or unsafe configs are rejected before anything is allocated", "[persistence]") {
    using Catch::Matchers::ContainsSubstring;
    auto config = [](const char* text) { return nlohmann::json::parse(text); };

    REQUIRE_THROWS_WITH(eco::configFromJson(config(R"({"gridWidth": 0})")), ContainsSubstring("grid size"));
    REQUIRE_THROWS_WITH(eco::configFromJson(config(R"({"gridHeight": -4})")), ContainsSubstring("grid size"));
    REQUIRE_THROWS_WITH(eco::configFromJson(config(R"({"gridWidth": 100000, "gridHeight": 100000})")),
                        ContainsSubstring("grid size"));
    REQUIRE_THROWS_WITH(eco::configFromJson(config(R"({"initial": {"prey": 99999999999}})")),
                        ContainsSubstring("populations"));
    REQUIRE_THROWS_WITH(eco::configFromJson(config(R"({"vegetation": {"seasonalPeriod": 0}})")),
                        ContainsSubstring("seasonalPeriod"));
    REQUIRE_THROWS_WITH(eco::configFromJson(config(R"({"vegetation": {"temperatureTolerance": 0}})")),
                        ContainsSubstring("temperatureTolerance"));
    REQUIRE_THROWS_WITH(eco::configFromJson(config(R"({"biomes": {"latticeSpacing": 0.001}})")),
                        ContainsSubstring("latticeSpacing"));

    // The same protection applies to a save file's embedded config.
    nlohmann::json save = eco::saveState(eco::buildSimulation(richScenario()));
    save["config"]["gridWidth"] = 2000000000;
    REQUIRE_THROWS_AS(eco::loadState(save), std::runtime_error);
}

// Fuzzing: corrupt a valid save in thousands of random ways (replace nodes with the wrong
// type, huge/negative numbers, empty containers...). Loading must either succeed and yield
// a simulation that can tick, or throw std::runtime_error -- never crash, and never leak
// a library-specific exception. (Run under ASan/UBSan this also proves memory safety; the
// original run of this fuzzer found a real contract bug: raw nlohmann exceptions and
// bad_alloc escaped instead of runtime_error.)
TEST_CASE("Corrupted save files never crash the loader or leak foreign exceptions",
          "[persistence][fuzz]") {
    eco::SimulationConfig config;
    config.gridWidth = 12;
    config.gridHeight = 12;
    config.initial = {60, 30, 5, 3};
    config.biomes.enabled = true;
    eco::Simulation sim = runFor(eco::buildSimulation(config), 40);
    const nlohmann::json good = eco::saveState(sim);

    std::function<void(nlohmann::json&, std::vector<nlohmann::json*>&)> collect =
        [&](nlohmann::json& node, std::vector<nlohmann::json*>& out) {
            out.push_back(&node);
            if (node.is_structured()) for (auto& child : node) collect(child, out);
        };

    std::mt19937 rng(12345);
    int accepted = 0, rejected = 0;
    for (int iteration = 0; iteration < 800; ++iteration) {
        nlohmann::json corrupted = good;
        for (int mutation = 0, count = 1 + static_cast<int>(rng() % 3); mutation < count; ++mutation) {
            // Re-collect after each mutation: overwriting a container frees its children.
            std::vector<nlohmann::json*> nodes;
            collect(corrupted, nodes);
            nlohmann::json* node = nodes[rng() % nodes.size()];
            switch (rng() % 7) {
            case 0: *node = nullptr; break;
            case 1: *node = "garbage"; break;
            case 2: *node = -1; break;
            case 3: *node = 1e30; break;
            case 4: *node = nlohmann::json::array(); break;
            case 5: *node = nlohmann::json::object(); break;
            default: *node = static_cast<int>(rng() % 100000) - 50000; break;
            }
        }
        try {
            eco::Simulation loaded = eco::loadState(corrupted);
            loaded.tick(1.0f / 30.0f); // even a file that loads must not be able to break a tick
            ++accepted;
        } catch (const std::runtime_error&) {
            ++rejected;
        }
        // Any other exception type propagates out of the try and fails the test.
    }
    REQUIRE(accepted + rejected == 800);
    REQUIRE(rejected > 400); // most random corruptions really are invalid
}
