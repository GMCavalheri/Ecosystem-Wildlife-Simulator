#include <cmath>
#include <cstddef>
#include <functional>
#include <numeric>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/WeightedSampling.h"

namespace {

// Exact probability that candidate `target` is among the `k` picks of sequential
// weighted sampling without replacement, by brute-force recursion over the pick order:
// each pick is drawn from the remaining candidates in proportion to their weights.
double exactInclusionProbability(const std::vector<double>& weights, std::size_t k,
                                 std::size_t target) {
    std::vector<bool> used(weights.size(), false);
    std::function<double(std::size_t, double)> recurse = [&](std::size_t picksLeft,
                                                            double pathProbability) -> double {
        if (picksLeft == 0) {
            return used[target] ? pathProbability : 0.0;
        }
        double remainingWeight = 0.0;
        for (std::size_t i = 0; i < weights.size(); ++i) {
            if (!used[i]) remainingWeight += weights[i];
        }
        double total = 0.0;
        for (std::size_t i = 0; i < weights.size(); ++i) {
            if (used[i]) continue;
            used[i] = true;
            total += recurse(picksLeft - 1, pathProbability * weights[i] / remainingWeight);
            used[i] = false;
        }
        return total;
    };
    return recurse(k, 1.0);
}

// Empirical inclusion frequency of each candidate over many independent draws.
std::vector<double> empiricalInclusion(const std::vector<double>& weights, std::size_t k,
                                       int trials, unsigned seed) {
    entt::registry registry;
    std::vector<entt::entity> candidates;
    std::vector<double> cumulative;
    double running = 0.0;
    for (double w : weights) {
        candidates.push_back(registry.create());
        running += w;
        cumulative.push_back(running);
    }

    std::mt19937 rng(seed);
    std::vector<int> hits(weights.size(), 0);
    for (int t = 0; t < trials; ++t) {
        for (auto entity : eco::weightedSampleWithoutReplacement(candidates, cumulative, k, rng)) {
            for (std::size_t i = 0; i < candidates.size(); ++i) {
                if (candidates[i] == entity) ++hits[i];
            }
        }
    }
    std::vector<double> frequency;
    for (int h : hits) frequency.push_back(static_cast<double>(h) / trials);
    return frequency;
}

} // namespace

// Phase 7 correctness gate: PredationSystem's victim selection was rewritten for speed
// (cumulative-weight sampling replaced a per-prey pow() + random draw). Both code paths
// in weightedSampleWithoutReplacement -- the fast cumulative path (k <= N/2) and the
// Efraimidis-Spirakis path (k > N/2) -- must reproduce the exact sequential-sampling
// inclusion probabilities, so "faster prey survive more" (Phase 3) is unchanged.
TEST_CASE("Weighted sampling without replacement matches exact inclusion probabilities",
          "[weighted-sampling]") {
    const std::vector<double> weights = {1.0, 2.0, 3.0, 0.5, 4.0, 1.5}; // N = 6
    constexpr int trials = 200000;

    SECTION("cumulative path (k=2 <= N/2)") {
        const auto frequency = empiricalInclusion(weights, 2, trials, 1u);
        for (std::size_t i = 0; i < weights.size(); ++i) {
            REQUIRE(frequency[i] ==
                    Catch::Approx(exactInclusionProbability(weights, 2, i)).margin(0.006));
        }
    }

    SECTION("Efraimidis-Spirakis path (k=4 > N/2)") {
        const auto frequency = empiricalInclusion(weights, 4, trials, 2u);
        for (std::size_t i = 0; i < weights.size(); ++i) {
            REQUIRE(frequency[i] ==
                    Catch::Approx(exactInclusionProbability(weights, 4, i)).margin(0.006));
        }
    }

    SECTION("always returns exactly k distinct candidates") {
        entt::registry registry;
        std::vector<entt::entity> candidates;
        std::vector<double> cumulative;
        double running = 0.0;
        for (double w : weights) {
            candidates.push_back(registry.create());
            running += w;
            cumulative.push_back(running);
        }
        std::mt19937 rng(3u);
        for (std::size_t k : {std::size_t{0}, std::size_t{1}, std::size_t{3}, std::size_t{6}}) {
            auto picked = eco::weightedSampleWithoutReplacement(candidates, cumulative, k, rng);
            REQUIRE(picked.size() == k);
            std::sort(picked.begin(), picked.end());
            REQUIRE(std::adjacent_find(picked.begin(), picked.end()) == picked.end());
        }
    }
}
