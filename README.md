# Ecosystem / Wildlife Simulator

A C++ ecosystem simulation aiming for RimWorld-style **emergent, non-deterministic
behavior**: no scripted events, only independent systems (terrain, populations,
hunger, disease, migration) whose *interactions* produce the story. First entry in a
planned series of simulation games (farm, industry, crime, ...).

## Design Pillars

1. **No scripted outcomes.** Every event (boom, crash, extinction) falls out of rules,
   never a hand-triggered script.
2. **Validate against real math first.** A 2-species (predator/prey) run with no
   terrain must reproduce classic Lotka-Volterra oscillations before adding
   complexity.
3. **Data-oriented over object-oriented.** Contiguous arrays of components over deep
   class hierarchies, so large populations (10k+ agents) stay tractable.
4. **Simulation and rendering are decoupled.** The core sim runs headless;
   visualization is an optional layer on top.

## Tech Stack

| Purpose | Library |
|---|---|
| Language/standard | C++20 |
| Build system | CMake + vcpkg |
| ECS | [EnTT](https://github.com/skypjack/entt) |
| Math | [GLM](https://github.com/g-truc/glm) *(render feature)* |
| Rendering | [raylib](https://www.raylib.com/) *(render feature)* |
| Debug UI / graphs | [Dear ImGui](https://github.com/ocornut/imgui) + [ImPlot](https://github.com/epezent/implot) *(render feature)* |
| Logging | [spdlog](https://github.com/gabime/spdlog) |
| Testing | [Catch2](https://github.com/catchorg/Catch2) |
| Serialization | [nlohmann/json](https://github.com/nlohmann/json) *(persistence feature)* |

## Building

Prerequisites: CMake 3.21+, a C++20 compiler, and [vcpkg](https://vcpkg.io) with
`VCPKG_ROOT` set in your environment.

```bash
cmake --preset default
cmake --build --preset default
ctest --preset default
```

Optional feature sets (rendering, persistence) can be enabled with vcpkg manifest
features once those phases are implemented, e.g.:

```bash
cmake --preset default -DVCPKG_MANIFEST_FEATURES="render"
```

## Architecture

- `src/components/` — plain-data ECS components ([Components.h](src/components/Components.h)): `Position`, `Species`, `Energy`, `Age`, `GeneticTraits`, `Health`, `Reproductive`.
- `src/environment/` — the terrain grid ([Grid.h](src/environment/Grid.h)), a flat `std::vector<Cell>` kept outside the ECS for cache locality.
- `src/systems/` — one class per system, run in a fixed order each tick: `EnvironmentSystem` -> `ForagingSystem` -> `PredationSystem` -> `ReproductionSystem` -> `DiseaseSystem` -> `MigrationSystem` -> `MortalitySystem`.
- `src/core/` — [`Simulation`](src/core/Simulation.h) owns the `entt::registry` and `Grid` and drives the tick loop; `MetricsRecorder` snapshots state for validation.
- `src/render/` — optional visualization layer (raylib + ImGui/ImPlot), only linked in when the `render` feature is enabled. Empty until Phase 6.
- `tests/` — Catch2 unit tests, especially for the math-heavy growth/predation logic.
- `data/` — config JSON (species params, tunables), added starting Phase 8.
- `tools/` — CSV-to-plot scripts for validating population dynamics (Phase 1).

## Build Phases

- **Phase 0 -- Scaffolding** (this commit): CMake + vcpkg, EnTT/spdlog/Catch2 wired in, empty headless tick loop, `ctest` target.
- **Phase 1 -- Two-species validation**: predator + prey, no terrain, population counts logged to CSV and checked against Lotka-Volterra oscillation. Correctness gate before anything else.
- **Phase 2 -- Spatial grid + vegetation**: `Cell` grid, `ForagingSystem`, `EnvironmentSystem` regrowth/seasons.
- **Phase 3 -- Genetics**: per-individual `GeneticTraits` mutation on reproduction; check trait drift under selection pressure.
- **Phase 4 -- Disease (SIR)**: `Health`/infection system, validated against known SIR dynamics.
- **Phase 5 -- Migration**: gradient-following movement; watch for clustering/resource collapse.
- **Phase 6 -- Visualization**: raylib grid renderer + ImGui/ImPlot live population graphs and tunable parameters.
- **Phase 7 -- Performance**: profile at 10k+ agents, parallelize independent systems, spatial partitioning for neighbor queries.
- **Phase 8 -- Expansion**: more species, seasonal migration, multiple biomes, save/load via JSON.

## Open Decisions

Tracked here until resolved in an implementation pass:

- **Agent-based vs. population-based for v1.** Agent-based (every animal is an ECS
  entity) is richer/more emergent but more expensive. Current plan: start
  population-based for the Phase 1 Lotka-Volterra validation, then move to
  agent-based once the math is trusted.
- **Grid size / target population scale**, which determines whether naive O(n^2)
  neighbor checks are acceptable early on or spatial partitioning is needed from
  day one.
- **Seasons/weather**: deterministic cycles vs. a light stochastic process.
