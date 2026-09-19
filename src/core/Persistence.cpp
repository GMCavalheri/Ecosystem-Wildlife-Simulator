#include "core/Persistence.h"

#include <fstream>
#include <format>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "components/Components.h"
#include "components/PreyGroup.h"
#include "environment/Biomes.h"

namespace eco {

using nlohmann::json;

// _WITH_DEFAULT: a missing key keeps the struct's default value, which is what makes
// partial config files work.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(VegetationParams, regrowthRate, baseTemperature,
                                                seasonalAmplitude, seasonalPeriod,
                                                optimalTemperature, temperatureTolerance,
                                                latitudeGradient)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ForagingParams, maxIntakeRate, energyPerVegetation,
                                                metabolicRate)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ReproductionParams, energyThreshold, attemptRate,
                                                offspringEnergy, parentEnergyCost)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GeneticsParams, speedMutationStdDev,
                                                sizeMutationStdDev, fertilityMutationStdDev,
                                                minTraitValue, maxTraitValue)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MigrationParams, moveAttemptRate,
                                                minVegetationAdvantage)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PredatorParams, predationRate, energyPerKill,
                                                metabolicRate, reproductionThreshold,
                                                reproductionAttemptRate, offspringEnergy,
                                                parentEnergyCost)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DiseaseParams, transmissionRate, recoveryRate,
                                                diseaseDeathRate)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InitialPopulation, prey, competitors, predators,
                                                infected)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BiomeConfig, enabled, seed, latticeSpacing)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SimulationConfig, gridWidth, gridHeight, seed,
                                                predator, vegetation, foraging, reproduction,
                                                competitorForaging, competitorReproduction,
                                                genetics, migration, disease, initial, biomes)

namespace {

// Every key in `given` must exist in `reference` (recursively), or it is a typo.
void rejectUnknownKeys(const json& given, const json& reference, const std::string& path) {
    if (!given.is_object() || !reference.is_object()) {
        return;
    }
    for (auto it = given.begin(); it != given.end(); ++it) {
        if (!reference.contains(it.key())) {
            throw std::runtime_error("unknown config key '" + path + it.key() + "'");
        }
        rejectUnknownKeys(it.value(), reference.at(it.key()), path + it.key() + ".");
    }
}

[[noreturn]] void fail(const std::string& message) { throw std::runtime_error("save file: " + message); }

template <typename T>
T field(const json& object, const char* key) {
    if (!object.is_object() || !object.contains(key)) {
        fail(std::string("missing field '") + key + "'");
    }
    try {
        return object.at(key).get<T>();
    } catch (const json::exception& e) {
        fail(std::string("field '") + key + "' has the wrong type (" + e.what() + ")");
    }
}

constexpr std::size_t kHerbivoreRowSize = 12;
constexpr std::size_t kPredatorRowSize = 2;
constexpr std::size_t kCellRowSize = 6;

} // namespace

json toJson(const SimulationConfig& config) { return json(config); }

void validateConfig(const SimulationConfig& c) {
    constexpr long long kMaxSide = 16384;
    constexpr long long kMaxCells = 64LL * 1024 * 1024;
    constexpr std::size_t kMaxAnimals = 10'000'000;
    auto bad = [](const std::string& why) { throw std::runtime_error("invalid config: " + why); };

    if (c.gridWidth < 1 || c.gridHeight < 1 || c.gridWidth > kMaxSide || c.gridHeight > kMaxSide ||
        static_cast<long long>(c.gridWidth) * c.gridHeight > kMaxCells) {
        bad("grid size must be between 1x1 and " + std::to_string(kMaxSide) + " per side (at most " +
            std::to_string(kMaxCells) + " cells), got " + std::to_string(c.gridWidth) + "x" +
            std::to_string(c.gridHeight));
    }
    if (c.initial.prey > kMaxAnimals || c.initial.competitors > kMaxAnimals ||
        c.initial.predators > kMaxAnimals) {
        bad("initial populations are limited to " + std::to_string(kMaxAnimals) + " per species");
    }
    if (!(c.vegetation.seasonalPeriod > 0.0f)) bad("vegetation.seasonalPeriod must be > 0");
    if (!(c.vegetation.temperatureTolerance > 0.0f)) bad("vegetation.temperatureTolerance must be > 0");
    if (!(c.foraging.energyPerVegetation > 0.0f) || !(c.competitorForaging.energyPerVegetation > 0.0f)) {
        bad("energyPerVegetation must be > 0");
    }
    if (!(c.biomes.latticeSpacing >= 1.0f)) bad("biomes.latticeSpacing must be >= 1");
}

namespace {

// Rewrites every floating-point number as the shortest decimal that round-trips its float.
void shortenFloats(json& node) {
    if (node.is_number_float()) {
        const float f = static_cast<float>(node.get<double>());
        node = std::stod(std::format("{}", f));
    } else if (node.is_object() || node.is_array()) {
        for (auto& child : node) {
            shortenFloats(child);
        }
    }
}

} // namespace

json toReadableJson(const SimulationConfig& config) {
    json out = toJson(config);
    shortenFloats(out);
    return out;
}

SimulationConfig configFromJson(const json& given) {
    rejectUnknownKeys(given, toJson(SimulationConfig{}), "");
    SimulationConfig config;
    try {
        config = given.get<SimulationConfig>();
    } catch (const json::exception& e) {
        throw std::runtime_error(std::string("invalid config: ") + e.what());
    }
    validateConfig(config);
    return config;
}

SimulationConfig loadConfigFile(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open config file: " + path.string());
    }
    try {
        return configFromJson(json::parse(in));
    } catch (const json::parse_error& e) {
        throw std::runtime_error("config file " + path.string() + " is not valid JSON: " + e.what());
    }
}

Simulation buildSimulation(const SimulationConfig& config) {
    validateConfig(config);
    Simulation sim(config);
    if (config.biomes.enabled) {
        generateBiomes(sim.grid(), config.biomes.seed, config.biomes.latticeSpacing);
    }
    sim.seedPopulation(kPreySpeciesId, config.initial.prey);
    sim.seedPopulation(kCompetitorSpeciesId, config.initial.competitors);
    sim.seedPopulation(kPredatorSpeciesId, config.initial.predators);
    sim.infectRandomPrey(config.initial.infected);
    return sim;
}

json saveState(const Simulation& sim) {
    json out;
    out["version"] = kSaveFormatVersion;
    out["config"] = toJson(sim.config());
    out["time"] = sim.simulationTime();
    out["rng"] = sim.rngState();

    const Grid& grid = sim.grid();
    json cells = json::array();
    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const Cell& c = grid.at(x, y);
            cells.push_back({c.vegetationDensity, c.waterAvailability, c.temperature, c.capacity,
                             c.growthMultiplier, static_cast<int>(c.biome)});
        }
    }
    out["grid"] = {{"width", grid.width()}, {"height", grid.height()}, {"cells", std::move(cells)}};

    // Animals in the order the systems iterate them.
    const entt::registry& registry = sim.registry();
    json herbivores = json::array();
    if (auto prey = preyGroup(registry)) {
        for (auto [entity, position, energy, traits, health, species] : prey.each()) {
            herbivores.push_back({species.id, position.cellX, position.cellY, energy.value,
                                  energy.max, traits.speed, traits.size, traits.fertility,
                                  health.value, health.infected ? 1 : 0, health.infectionTimer,
                                  health.immune ? 1 : 0});
        }
    }
    out["herbivores"] = std::move(herbivores);

    json predators = json::array();
    for (auto [entity, species, energy] :
         registry.view<const Species, const Energy>(entt::exclude<GeneticTraits>).each()) {
        if (species.id == kPredatorSpeciesId) {
            predators.push_back({energy.value, energy.max});
        }
    }
    out["predators"] = std::move(predators);
    return out;
}

namespace {

Simulation loadStateUnchecked(const json& in) {
    if (field<int>(in, "version") != kSaveFormatVersion) {
        fail("unsupported version " + std::to_string(field<int>(in, "version")) +
             " (this build reads version " + std::to_string(kSaveFormatVersion) + ")");
    }

    SimulationConfig config;
    try {
        config = in.at("config").get<SimulationConfig>();
        validateConfig(config);
    } catch (const json::exception& e) {
        fail(std::string("invalid config section (") + e.what() + ")");
    } catch (const std::runtime_error& e) {
        fail(std::string("invalid config section (") + e.what() + ")");
    }
    Simulation sim(config);
    sim.restoreSimulationTime(field<float>(in, "time"));
    sim.restoreRngState(field<std::string>(in, "rng"));

    // Terrain.
    const json& gridJson = in.at("grid");
    Grid& grid = sim.grid();
    if (field<int>(gridJson, "width") != grid.width() || field<int>(gridJson, "height") != grid.height()) {
        fail("grid size does not match the config");
    }
    const json& cells = gridJson.at("cells");
    if (!cells.is_array() || cells.size() != static_cast<std::size_t>(grid.width()) * grid.height()) {
        fail("wrong number of grid cells");
    }
    std::size_t index = 0;
    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x, ++index) {
            const json& row = cells[index];
            if (!row.is_array() || row.size() != kCellRowSize) fail("malformed grid cell");
            Cell& c = grid.at(x, y);
            c.vegetationDensity = row[0].get<float>();
            c.waterAvailability = row[1].get<float>();
            c.temperature = row[2].get<float>();
            c.capacity = row[3].get<float>();
            c.growthMultiplier = row[4].get<float>();
            const int biome = row[5].get<int>();
            if (biome < 0 || biome > static_cast<int>(Biome::Wetland)) fail("unknown biome id");
            c.biome = static_cast<Biome>(biome);
        }
    }

    // Animals. The file lists them in the order the systems *iterate* them, and EnTT
    // iterates the most recently created entity first, so they are recreated in reverse:
    // that makes the loaded simulation iterate in exactly the saved order.
    entt::registry& registry = sim.registry();
    const json& herbivoreRows = in.at("herbivores");
    for (auto rowIt = herbivoreRows.rbegin(); rowIt != herbivoreRows.rend(); ++rowIt) {
        const json& row = *rowIt;
        if (!row.is_array() || row.size() != kHerbivoreRowSize) fail("malformed herbivore row");
        const SpeciesId species = row[0].get<SpeciesId>();
        if (species != kPreySpeciesId && species != kCompetitorSpeciesId) fail("unknown herbivore species");
        const int x = row[1].get<int>();
        const int y = row[2].get<int>();
        if (x < 0 || x >= grid.width() || y < 0 || y >= grid.height()) fail("herbivore outside the grid");

        auto entity = registry.create();
        registry.emplace<Species>(entity, species);
        registry.emplace<Position>(entity, x, y);
        registry.emplace<Energy>(entity, row[3].get<float>(), row[4].get<float>());
        registry.emplace<GeneticTraits>(entity, row[5].get<float>(), row[6].get<float>(),
                                        row[7].get<float>());
        registry.emplace<Health>(entity, row[8].get<float>(), row[9].get<int>() != 0,
                                 row[10].get<float>(), row[11].get<int>() != 0);
    }
    const json& predatorRows = in.at("predators");
    for (auto rowIt = predatorRows.rbegin(); rowIt != predatorRows.rend(); ++rowIt) {
        const json& row = *rowIt;
        if (!row.is_array() || row.size() != kPredatorRowSize) fail("malformed predator row");
        auto entity = registry.create();
        registry.emplace<Species>(entity, kPredatorSpeciesId);
        registry.emplace<Energy>(entity, row[0].get<float>(), row[1].get<float>());
    }

    // Start the resumed run's history with the state it resumes from.
    sim.metrics().snapshot(registry, grid, sim.simulationTime());
    return sim;
}

} // namespace

Simulation loadState(const json& in) {
    // Whatever goes wrong in a bad file -- a missing key, a value of the wrong type, an
    // absurd size -- surfaces as std::runtime_error, never a library-specific exception.
    try {
        return loadStateUnchecked(in);
    } catch (const json::exception& e) {
        fail(std::string("malformed contents (") + e.what() + ")");
    } catch (const std::bad_alloc&) {
        fail("contents describe more data than can be allocated");
    } catch (const std::length_error&) {
        fail("contents describe more data than can be allocated");
    }
}

void saveStateToFile(const Simulation& sim, const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("cannot write save file: " + path.string());
    }
    out << saveState(sim).dump();
}

Simulation loadStateFromFile(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open save file: " + path.string());
    }
    try {
        return loadState(json::parse(in));
    } catch (const json::parse_error& e) {
        throw std::runtime_error("save file " + path.string() + " is not valid JSON: " + e.what());
    } catch (const json::exception& e) {
        throw std::runtime_error("save file " + path.string() + " is malformed: " + e.what());
    }
}

} // namespace eco
