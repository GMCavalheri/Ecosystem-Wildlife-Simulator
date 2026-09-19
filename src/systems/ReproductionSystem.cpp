#include "systems/ReproductionSystem.h"

#include <algorithm>
#include <vector>

#include "components/Components.h"
#include "components/PreyGroup.h"

namespace eco {

namespace {

float mutate(float parentValue, float stdDev, float minValue, float maxValue,
             std::mt19937& rng) {
    std::normal_distribution<float> noise(0.0f, stdDev);
    return std::clamp(parentValue + noise(rng), minValue, maxValue);
}

} // namespace

void ReproductionSystem::update(entt::registry& registry, std::mt19937& rng, float dt,
                                 const ReproductionParams& preyReproParams,
                                 const ReproductionParams& competitorReproParams,
                                 const GeneticsParams& geneticsParams,
                                 const PredatorParams& predatorParams) {
    // One scan finds both species' eligible parents (this used to be two full passes).
    eligiblePrey_.clear();
    eligiblePredators_.clear();
    for (auto [entity, position, energy, traits, health, species] : preyGroup(registry).each()) {
        const ReproductionParams& params =
            species.id == kCompetitorSpeciesId ? competitorReproParams : preyReproParams;
        if (energy.value >= params.energyThreshold) {
            eligiblePrey_.push_back(entity);
        }
    }
    // Predators: the (few) entities with an Energy but no GeneticTraits.
    for (auto [entity, species, energy] :
         registry.view<Species, Energy>(entt::exclude<GeneticTraits>).each()) {
        if (species.id == kPredatorSpeciesId &&
            energy.value >= predatorParams.reproductionThreshold) {
            eligiblePredators_.push_back(entity);
        }
    }

    // Prey: Energy-gated reproduction with mutated GeneticTraits, at the parent's cell.

    for (auto entity : eligiblePrey_) {
        const SpeciesId speciesId = registry.get<Species>(entity).id;
        const ReproductionParams& reproParams =
            speciesId == kCompetitorSpeciesId ? competitorReproParams : preyReproParams;
        // Each species has its own attempt rate; the distribution is cheap to build.
        std::bernoulli_distribution preyAttempt(
            std::clamp(static_cast<double>(reproParams.attemptRate) * dt, 0.0, 1.0));
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
        registry.emplace<Species>(offspring, speciesId);
        registry.emplace<Position>(offspring, parentPosition);
        registry.emplace<Energy>(offspring, reproParams.offspringEnergy, 100.0f);
        registry.emplace<GeneticTraits>(offspring, offspringTraits);
        registry.emplace<Health>(offspring, 100.0f, false, 0.0f, false);
    }

    // Predators: same Energy-gated pattern, mirroring prey's resilience mechanic (see
    // PredatorParams for why). No Position/GeneticTraits yet -- predation is still
    // mean-field/non-spatial.
    std::bernoulli_distribution predatorAttempt(std::clamp(
        static_cast<double>(predatorParams.reproductionAttemptRate) * dt, 0.0, 1.0));

    for (auto entity : eligiblePredators_) {
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
