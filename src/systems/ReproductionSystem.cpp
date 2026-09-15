#include "systems/ReproductionSystem.h"

#include <algorithm>
#include <vector>

#include "components/Components.h"

namespace eco {

namespace {

float mutate(float parentValue, float stdDev, float minValue, float maxValue,
             std::mt19937& rng) {
    std::normal_distribution<float> noise(0.0f, stdDev);
    return std::clamp(parentValue + noise(rng), minValue, maxValue);
}

} // namespace

void ReproductionSystem::update(entt::registry& registry, std::mt19937& rng, float dt,
                                 const ReproductionParams& reproParams,
                                 const GeneticsParams& geneticsParams) {
    std::vector<entt::entity> eligible;
    auto view = registry.view<Species, Energy>();
    for (auto entity : view) {
        if (view.get<Species>(entity).id == kPreySpeciesId &&
            view.get<Energy>(entity).value >= reproParams.energyThreshold) {
            eligible.push_back(entity);
        }
    }

    std::bernoulli_distribution attempt(
        std::clamp(static_cast<double>(reproParams.attemptRate) * dt, 0.0, 1.0));

    for (auto entity : eligible) {
        if (!attempt(rng)) {
            continue;
        }

        auto& parentEnergy = registry.get<Energy>(entity);
        if (parentEnergy.value < reproParams.parentEnergyCost) {
            continue;
        }
        parentEnergy.value -= reproParams.parentEnergyCost;

        const auto parentPosition = registry.get<Position>(entity);
        const auto& parentTraits = registry.get<GeneticTraits>(entity);

        GeneticTraits offspringTraits;
        offspringTraits.speed =
            mutate(parentTraits.speed, geneticsParams.speedMutationStdDev,
                   geneticsParams.minTraitValue, geneticsParams.maxTraitValue, rng);
        offspringTraits.size =
            mutate(parentTraits.size, geneticsParams.sizeMutationStdDev,
                   geneticsParams.minTraitValue, geneticsParams.maxTraitValue, rng);
        offspringTraits.fertility =
            mutate(parentTraits.fertility, geneticsParams.fertilityMutationStdDev,
                   geneticsParams.minTraitValue, geneticsParams.maxTraitValue, rng);

        auto offspring = registry.create();
        registry.emplace<Species>(offspring, kPreySpeciesId);
        registry.emplace<Position>(offspring, parentPosition);
        registry.emplace<Energy>(offspring, reproParams.offspringEnergy, 100.0f);
        registry.emplace<GeneticTraits>(offspring, offspringTraits);
    }
}

} // namespace eco
