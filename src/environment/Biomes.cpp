#include "environment/Biomes.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

namespace eco {

namespace {

float smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }

} // namespace

void generateBiomes(Grid& grid, unsigned seed, float latticeSpacing) {
    const int width = grid.width();
    const int height = grid.height();
    const int latticeW = static_cast<int>(std::ceil(width / latticeSpacing)) + 2;
    const int latticeH = static_cast<int>(std::ceil(height / latticeSpacing)) + 2;

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
    std::vector<float> lattice(static_cast<std::size_t>(latticeW) * latticeH);
    for (auto& v : lattice) {
        v = uniform(rng);
    }
    auto latticeAt = [&](int i, int j) {
        return lattice[static_cast<std::size_t>(j) * latticeW + static_cast<std::size_t>(i)];
    };

    std::vector<float> moisture(static_cast<std::size_t>(width) * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float fx = x / latticeSpacing;
            const float fy = y / latticeSpacing;
            const int i = static_cast<int>(fx);
            const int j = static_cast<int>(fy);
            const float tx = smoothstep(fx - i);
            const float ty = smoothstep(fy - j);
            const float top = latticeAt(i, j) * (1 - tx) + latticeAt(i + 1, j) * tx;
            const float bottom = latticeAt(i, j + 1) * (1 - tx) + latticeAt(i + 1, j + 1) * tx;
            moisture[static_cast<std::size_t>(y) * width + x] = top * (1 - ty) + bottom * ty;
        }
    }

    // Rank cells by moisture and split by quantile so biome areas are fixed fractions.
    std::vector<std::size_t> order(moisture.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return moisture[a] < moisture[b];
    });

    const double total = static_cast<double>(order.size());
    for (std::size_t rank = 0; rank < order.size(); ++rank) {
        const double q = (static_cast<double>(rank) + 0.5) / total;
        const Biome biome = q < 0.15   ? Biome::Desert
                            : q < 0.50 ? Biome::Plains
                            : q < 0.85 ? Biome::Forest
                                       : Biome::Wetland;
        const std::size_t index = order[rank];
        const int x = static_cast<int>(index % static_cast<std::size_t>(width));
        const int y = static_cast<int>(index / static_cast<std::size_t>(width));

        const BiomeProperties props = biomeProperties(biome);
        Cell& cell = grid.at(x, y);
        cell.biome = biome;
        cell.capacity = props.capacity;
        cell.growthMultiplier = props.growthMultiplier;
        cell.waterAvailability = moisture[index];
        cell.vegetationDensity = props.capacity;
    }
}

} // namespace eco
