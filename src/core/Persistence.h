#pragma once

#include <filesystem>

#include <nlohmann/json_fwd.hpp>

#include "core/Simulation.h"
#include "core/SimulationConfig.h"

namespace eco {

// Phase 8: JSON configuration and save/load.
//
// Config files are partial by design: any key you leave out keeps its default, so a
// scenario file only needs to list what it changes. Unknown keys are an error (a typo
// like "regrowthrate" that silently did nothing is a nasty failure mode).
//
// A save file captures everything a run depends on -- parameters, the clock, the RNG's
// full state, the terrain, and every animal *in the order the systems visit them*
// (prey that share a cell forage one after another, so order is part of the state).
// Resuming from a save therefore continues bit-identically to a run that never stopped.
// Not saved: the metrics history (a resumed run starts a fresh one) and the thread
// count (a runtime choice that never affects results).

inline constexpr int kSaveFormatVersion = 1;

nlohmann::json toJson(const SimulationConfig& config);

// Same content as toJson(), but with every number printed as the shortest decimal that
// round-trips its 32-bit float (0.12 rather than 0.11999999731779099) -- for files a
// person will read and edit. Save files use toJson(), which is exact by construction.
nlohmann::json toReadableJson(const SimulationConfig& config);

// Throws std::runtime_error if a config is unusable *before* anything is allocated from
// it: a grid or population of absurd size, or a parameter that would divide by zero.
void validateConfig(const SimulationConfig& config);

// Throws std::runtime_error on unknown keys, values of the wrong type, or an invalid
// configuration (see validateConfig).
SimulationConfig configFromJson(const nlohmann::json& json);
SimulationConfig loadConfigFile(const std::filesystem::path& path);

// A fresh scenario: the config's world, biome map and starting populations.
Simulation buildSimulation(const SimulationConfig& config);

nlohmann::json saveState(const Simulation& simulation);

// Throws std::runtime_error on a wrong version, missing fields, or invalid contents
// (out-of-range positions, unknown species, a grid that doesn't match the config).
Simulation loadState(const nlohmann::json& json);

void saveStateToFile(const Simulation& simulation, const std::filesystem::path& path);
Simulation loadStateFromFile(const std::filesystem::path& path);

} // namespace eco
