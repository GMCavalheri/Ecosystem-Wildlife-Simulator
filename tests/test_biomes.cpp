#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/Simulation.h"
#include "environment/Biomes.h"

namespace {

std::array<int, 4> countBiomes(const eco::Grid& grid) {
    std::array<int, 4> counts{};
    for (int y = 0; y < grid.height(); ++y)
        for (int x = 0; x < grid.width(); ++x)
            ++counts[static_cast<std::size_t>(grid.at(x, y).biome)];
    return counts;
}

} // namespace

TEST_CASE("Biome generation is deterministic and seed-dependent", "[biomes]") {
    eco::Grid a(64, 64), b(64, 64), c(64, 64);
    eco::generateBiomes(a, 7u);
    eco::generateBiomes(b, 7u);
    eco::generateBiomes(c, 8u);

    bool sameAsB = true, sameAsC = true;
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            sameAsB = sameAsB && a.at(x, y).biome == b.at(x, y).biome &&
                      a.at(x, y).waterAvailability == b.at(x, y).waterAvailability;
            sameAsC = sameAsC && a.at(x, y).biome == c.at(x, y).biome;
        }
    }
    REQUIRE(sameAsB);
    REQUIRE_FALSE(sameAsC);
}

TEST_CASE("Every biome exists, in its intended share, with its intended properties",
          "[biomes]") {
    for (unsigned seed : {1u, 2u, 3u}) {
        eco::Grid grid(80, 60);
        eco::generateBiomes(grid, seed);
        const auto counts = countBiomes(grid);
        const double total = 80.0 * 60.0;

        // Quantile-split: shares are fixed regardless of seed (Desert 15, Plains 35,
        // Forest 35, Wetland 15 percent).
        REQUIRE(counts[static_cast<std::size_t>(eco::Biome::Desert)] / total == Catch::Approx(0.15).margin(0.005));
        REQUIRE(counts[static_cast<std::size_t>(eco::Biome::Plains)] / total == Catch::Approx(0.35).margin(0.005));
        REQUIRE(counts[static_cast<std::size_t>(eco::Biome::Forest)] / total == Catch::Approx(0.35).margin(0.005));
        REQUIRE(counts[static_cast<std::size_t>(eco::Biome::Wetland)] / total == Catch::Approx(0.15).margin(0.005));

        for (int y = 0; y < 60; ++y) {
            for (int x = 0; x < 80; ++x) {
                const auto& cell = grid.at(x, y);
                const auto props = eco::biomeProperties(cell.biome);
                REQUIRE(cell.capacity == props.capacity);
                REQUIRE(cell.growthMultiplier == props.growthMultiplier);
                REQUIRE(cell.vegetationDensity == props.capacity); // starts at carrying capacity
            }
        }
    }
}

// Biomes should form contiguous regions (smooth noise), not salt-and-pepper: neighboring
// cells' moisture should differ far less than randomly chosen cells' moisture does.
TEST_CASE("Biome regions are spatially coherent", "[biomes]") {
    eco::Grid grid(64, 64);
    eco::generateBiomes(grid, 11u);

    double neighborDiff = 0.0;
    int neighborPairs = 0;
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 63; ++x) {
            neighborDiff += std::abs(grid.at(x, y).waterAvailability - grid.at(x + 1, y).waterAvailability);
            ++neighborPairs;
        }
    }
    std::mt19937 rng(5);
    std::uniform_int_distribution<int> pick(0, 63);
    double randomDiff = 0.0;
    for (int i = 0; i < 4000; ++i) {
        randomDiff += std::abs(grid.at(pick(rng), pick(rng)).waterAvailability -
                               grid.at(pick(rng), pick(rng)).waterAvailability);
    }
    REQUIRE(neighborDiff / neighborPairs < 0.25 * randomDiff / 4000);
}

// Integration: same population, same rules, but prey concentrate where the vegetation
// can support them. After a settling period, biomes with higher carrying capacity hold
// more prey per cell than deserts.
TEST_CASE("Prey density follows biome carrying capacity", "[biomes]") {
    eco::PredatorParams noPredators;
    noPredators.predationRate = 0.0f;
    eco::MigrationParams noMigration; // isolate: density differences come from local
    noMigration.moveAttemptRate = 0.0f; // carrying capacity, not from walking around

    std::array<double, 4> perCell{}; // summed over seeds, indexed by Biome
    constexpr int seeds = 6;
    for (unsigned seed = 1; seed <= seeds; ++seed) {
        eco::Simulation sim(48, 48, noPredators, {}, {}, {}, {}, noMigration, {}, seed);
        eco::generateBiomes(sim.grid(), seed);
        sim.seedPopulation(eco::kPreySpeciesId, 1500);
        for (int i = 0; i < 300; ++i) sim.tick(1.0f / 30.0f);

        std::array<double, 4> prey{};
        for (auto [e, position] : sim.registry().view<const eco::Position>().each()) {
            prey[static_cast<std::size_t>(sim.grid().at(position.cellX, position.cellY).biome)] += 1.0;
        }
        const auto cells = countBiomes(sim.grid());
        for (std::size_t b = 0; b < 4; ++b) perCell[b] += prey[b] / cells[b];
    }

    const double desert = perCell[static_cast<std::size_t>(eco::Biome::Desert)];
    const double plains = perCell[static_cast<std::size_t>(eco::Biome::Plains)];
    const double forest = perCell[static_cast<std::size_t>(eco::Biome::Forest)];
    const double wetland = perCell[static_cast<std::size_t>(eco::Biome::Wetland)];

    // Measured per-cell density: Desert ~0.3, Plains ~1.3, Forest ~1.9, Wetland ~2.3, i.e.
    // the ordering of the biome table's carrying capacity x regrowth speed.
    REQUIRE(desert < plains);
    REQUIRE(plains < forest);
    REQUIRE(forest < wetland);
    REQUIRE(wetland > 4.0 * desert); // ~7-11x measured per seed
}
