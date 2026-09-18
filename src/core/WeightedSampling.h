#pragma once

#include <cstddef>
#include <random>
#include <vector>

#include <entt/entt.hpp>

namespace eco {

// Picks `count` distinct candidates with probability proportional to weight, without
// replacement (i.e. sequential weighted sampling: each pick is drawn from the
// remaining candidates in proportion to their weights). `cumulative` is the prefix sum
// of the candidates' weights (cumulative[i] = w[0] + ... + w[i]), which callers can
// build in the same pass that collects the candidates.
//
// Two strategies with the identical resulting distribution:
//  - count <= N/2 (the normal case, since kills per tick << prey): draw from the full
//    cumulative distribution by binary search and discard repeats -- O(k log N) after
//    the prefix sums, with no per-candidate random draw or pow().
//  - count > N/2 (where discarding repeats would thrash): Efraimidis-Spirakis keys
//    u^(1/weight), keeping the `count` largest.
std::vector<entt::entity> weightedSampleWithoutReplacement(
    const std::vector<entt::entity>& candidates, const std::vector<double>& cumulative,
    std::size_t count, std::mt19937& rng);

} // namespace eco
