#include "systems/PredationSystem.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "components/Components.h"

namespace eco {

namespace {

// Efraimidis-Spirakis weighted sampling without replacement: draw u ~ Uniform(0,1)
// per candidate, take key = u^(1/weight), keep the `count` largest keys. Higher
// weight (here, higher catchability = 1/speed) means a candidate is more likely to
// land among the top keys, without ever needing to normalize the weights.
std::vector<entt::entity> weightedSampleWithoutReplacement(
    const std::vector<entt::entity>& candidates, const std::vector<double>& weights,
    int count, std::mt19937& rng) {
    std::uniform_real_distribution<double> uniform01(0.0, 1.0);

    std::vector<std::pair<double, entt::entity>> keyed;
    keyed.reserve(candidates.size());
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const double u = uniform01(rng);
        const double key = std::pow(u, 1.0 / weights[i]);
        keyed.emplace_back(key, candidates[i]);
    }

    const auto middle = keyed.begin() + std::min<std::size_t>(count, keyed.size());
    std::partial_sort(keyed.begin(), middle, keyed.end(),
                       [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<entt::entity> selected;
    selected.reserve(count);
    for (auto it = keyed.begin(); it != middle; ++it) {
        selected.push_back(it->second);
    }
    return selected;
}

} // namespace

void PredationSystem::update(entt::registry& registry, std::mt19937& rng, float dt,
                              const LotkaVolterraParams& params) {
    std::vector<entt::entity> prey;
    std::vector<double> catchability;
    std::size_t predatorCount = 0;

    auto view = registry.view<Species>();
    for (auto entity : view) {
        if (view.get<Species>(entity).id == kPreySpeciesId) {
            const float speed = registry.get<GeneticTraits>(entity).speed;
            prey.push_back(entity);
            catchability.push_back(1.0 / std::max(speed, 1e-3f));
        } else if (view.get<Species>(entity).id == kPredatorSpeciesId) {
            ++predatorCount;
        }
    }

    if (prey.empty() || predatorCount == 0) {
        return;
    }

    const double expectedKills =
        static_cast<double>(params.predationRate) * static_cast<double>(prey.size()) *
        static_cast<double>(predatorCount) * static_cast<double>(dt);
    std::poisson_distribution<int> killDist(expectedKills);
    const int kills = std::min<int>(killDist(rng), static_cast<int>(prey.size()));

    const auto caught = weightedSampleWithoutReplacement(prey, catchability, kills, rng);
    for (auto entity : caught) {
        registry.destroy(entity);
    }

    if (kills > 0) {
        std::poisson_distribution<int> birthDist(
            static_cast<double>(params.conversionEfficiency) * kills);
        int predatorBirths = birthDist(rng);
        if (predatorCount + static_cast<std::size_t>(predatorBirths) > kMaxSpeciesPopulation) {
            predatorBirths = static_cast<int>(
                kMaxSpeciesPopulation - std::min(predatorCount, kMaxSpeciesPopulation));
        }
        for (int i = 0; i < predatorBirths; ++i) {
            auto entity = registry.create();
            registry.emplace<Species>(entity, kPredatorSpeciesId);
        }
    }
}

} // namespace eco
