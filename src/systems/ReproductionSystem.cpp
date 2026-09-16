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
                                 const GeneticsParams& geneticsParams,
                                 const PredatorParams& predatorParams) {
    // Prey: Energy-gated reproduction with mutated GeneticTraits, at the parent's cell.
    std::vector<entt::entity> eligiblePrey;
    auto view = registry.view<Species, Energy>();
    for (auto entity : view) {
        if (view.get<Species>(entity).id == kPreySpeciesId &&
            view.get<Energy>(entity).value >= reproParams.energyThreshold) {
            eligiblePrey.push_back(entity);
        }
    }

    std::bernoulli_distribution preyAttempt(
        std::clamp(static_cast<double>(reproParams.attemptRate) * dt, 0.0, 1.0));

    for (auto entity : eligiblePrey) {
        if (!preyAttempt(rng)) {
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

    // Predators: same Energy-gated pattern, mirroring prey's resilience mechanic (see
    // PredatorParams for why). No Position/GeneticTraits yet -- predation is still
    // mean-field/non-spatial.
    std::vector<entt::entity> eligiblePredators;
    for (auto entity : view) {
        if (view.get<Species>(entity).id == kPredatorSpeciesId &&
            view.get<Energy>(entity).value >= predatorParams.reproductionThreshold) {
            eligiblePredators.push_back(entity);
        }
    }

    std::bernoulli_distribution predatorAttempt(std::clamp(
        static_cast<double>(predatorParams.reproductionAttemptRate) * dt, 0.0, 1.0));

    for (auto entity : eligiblePredators) {
        if (!predatorAttempt(rng)) {
            continue;
        }

        auto& parentEnergy = registry.get<Energy>(entity);
        if (parentEnergy.value < predatorParams.parentEnergyCost) {
            continue;
        }
        parentEnergy.value -= predatorParams.parentEnergyCost;

        auto offspring = registry.create();
        registry.emplace<Species>(offspring, kPredatorSpeciesId);
        registry.emplace<Energy>(offspring, predatorParams.offspringEnergy, 100.0f);
    }
}

} // namespace eco
