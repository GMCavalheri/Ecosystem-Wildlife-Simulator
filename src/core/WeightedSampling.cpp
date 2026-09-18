#include "core/WeightedSampling.h"

#include <algorithm>
#include <cmath>

namespace eco {

std::vector<entt::entity> weightedSampleWithoutReplacement(
    const std::vector<entt::entity>& candidates, const std::vector<double>& cumulative,
    std::size_t count, std::mt19937& rng) {
    std::vector<entt::entity> selected;
    count = std::min(count, candidates.size());
    selected.reserve(count);
    if (count == 0) {
        return selected;
    }

    if (count * 2 <= candidates.size()) {
        std::uniform_real_distribution<double> uniform(0.0, cumulative.back());
        std::vector<std::size_t> chosen;
        chosen.reserve(count);
        while (chosen.size() < count) {
            const double target = uniform(rng);
            std::size_t index = static_cast<std::size_t>(
                std::upper_bound(cumulative.begin(), cumulative.end(), target) -
                cumulative.begin());
            index = std::min(index, candidates.size() - 1);
            if (std::find(chosen.begin(), chosen.end(), index) == chosen.end()) {
                chosen.push_back(index);
            }
        }
        for (const std::size_t index : chosen) {
            selected.push_back(candidates[index]);
        }
        return selected;
    }

    std::uniform_real_distribution<double> uniform01(0.0, 1.0);
    std::vector<std::pair<double, entt::entity>> keyed;
    keyed.reserve(candidates.size());
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const double weight = cumulative[i] - (i == 0 ? 0.0 : cumulative[i - 1]);
        keyed.emplace_back(std::pow(uniform01(rng), 1.0 / weight), candidates[i]);
    }
    const auto middle = keyed.begin() + count;
    std::partial_sort(keyed.begin(), middle, keyed.end(),
                       [](const auto& a, const auto& b) { return a.first > b.first; });
    for (auto it = keyed.begin(); it != middle; ++it) {
        selected.push_back(it->second);
    }
    return selected;
}

} // namespace eco
