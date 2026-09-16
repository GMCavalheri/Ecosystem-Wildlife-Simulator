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
                              const PredatorParams& params) {
    std::vector<entt::entity> prey;
    std::vector<double> catchability;
    std::vector<entt::entity> predators;

    auto view = registry.view<Species>();
    for (auto entity : view) {
        if (view.get<Species>(entity).id == kPreySpeciesId) {
            const float speed = registry.get<GeneticTraits>(entity).speed;
            prey.push_back(entity);
            catchability.push_back(1.0 / std::max(speed, 1e-3f));
        } else if (view.get<Species>(entity).id == kPredatorSpeciesId) {
            predators.push_back(entity);
        }
    }

    if (!prey.empty() && !predators.empty()) {
        const double expectedKills = static_cast<double>(params.predationRate) *
                                      static_cast<double>(prey.size()) *
                                      static_cast<double>(predators.size()) *
                                      static_cast<double>(dt);
        std::poisson_distribution<int> killDist(expectedKills);
        const int kills = std::min<int>(killDist(rng), static_cast<int>(prey.size()));

        const auto caught = weightedSampleWithoutReplacement(prey, catchability, kills, rng);
        for (auto entity : caught) {
            registry.destroy(entity);
        }

        // Each kill's biomass feeds one randomly-chosen living predator -- a lucky
        // predator can land more than one kill in the same tick.
        std::uniform_int_distribution<std::size_t> pickPredator(0, predators.size() - 1);
        for (int i = 0; i < kills; ++i) {
            auto& energy = registry.get<Energy>(predators[pickPredator(rng)]);
            energy.value = std::min(energy.max, energy.value + params.energyPerKill);
        }
    }

    // Metabolism applies every tick regardless of hunting success -- this is what
    // lets a predator coast through a lean patch instead of depending on that exact
    // tick's kill count.
    for (auto entity : predators) {
        auto& energy = registry.get<Energy>(entity);
        energy.value = std::max(0.0f, energy.value - params.metabolicRate * dt);
    }
}

} // namespace eco
