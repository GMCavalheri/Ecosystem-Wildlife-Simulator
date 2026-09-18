#include "systems/PredationSystem.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "components/Components.h"
#include "core/WeightedSampling.h"

namespace eco {

void PredationSystem::update(entt::registry& registry, std::mt19937& rng, float dt,
                              const PredatorParams& params) {
    // Predators are the (few) entities with an Energy but no GeneticTraits; every prey
    // has GeneticTraits (seedPopulation and ReproductionSystem both attach it), so the
    // prey count is just that pool's size -- no need to scan every entity to find it.
    predators_.clear();
    for (auto entity : registry.view<Species, Energy>(entt::exclude<GeneticTraits>)) {
        if (registry.get<Species>(entity).id == kPredatorSpeciesId) {
            predators_.push_back(entity);
        }
    }
    const std::size_t preyCount = registry.storage<GeneticTraits>().size();

    if (preyCount > 0 && !predators_.empty()) {
        const double expectedKills = static_cast<double>(params.predationRate) *
                                      static_cast<double>(preyCount) *
                                      static_cast<double>(predators_.size()) *
                                      static_cast<double>(dt);
        std::poisson_distribution<int> killDist(expectedKills);
        const std::size_t kills =
            std::min<std::size_t>(static_cast<std::size_t>(killDist(rng)), preyCount);

        if (kills > 0) {
            // Catchability = 1/speed: one pass builds the prefix sums.
            prey_.clear();
            cumulativeWeight_.clear();
            double running = 0.0;
            for (auto [entity, traits] : registry.view<GeneticTraits>().each()) {
                running += 1.0 / std::max(traits.speed, 1e-3f);
                prey_.push_back(entity);
                cumulativeWeight_.push_back(running);
            }

            const auto caught =
                weightedSampleWithoutReplacement(prey_, cumulativeWeight_, kills, rng);
            for (auto entity : caught) {
                registry.destroy(entity);
            }

            // Each kill's biomass feeds one randomly-chosen living predator -- a lucky
            // predator can land more than one kill in the same tick.
            std::uniform_int_distribution<std::size_t> pickPredator(0, predators_.size() - 1);
            for (std::size_t i = 0; i < kills; ++i) {
                auto& energy = registry.get<Energy>(predators_[pickPredator(rng)]);
                energy.value = std::min(energy.max, energy.value + params.energyPerKill);
            }
        }
    }

    // Metabolism applies every tick regardless of hunting success -- this is what
    // lets a predator coast through a lean patch instead of depending on that exact
    // tick's kill count.
    for (auto entity : predators_) {
        auto& energy = registry.get<Energy>(entity);
        energy.value = std::max(0.0f, energy.value - params.metabolicRate * dt);
    }
}

} // namespace eco
