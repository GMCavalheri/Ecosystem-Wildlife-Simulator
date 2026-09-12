#pragma once

#include <cstddef>

namespace eco {

// Phase 1: parameters for the mean-field (non-spatial) Lotka-Volterra predator/prey
// model used to validate core population dynamics before terrain, energy, and
// genetics are layered on. Fixed points: prey* = d / (b * c), predator* = a / b.
struct LotkaVolterraParams {
    float preyBirthRate = 1.0f;         // a: prey births per prey per unit time
    float predationRate = 0.005f;       // b: predation events per prey-predator pair per unit time
    float conversionEfficiency = 0.5f;  // c: predator births per successful predation
    float predatorDeathRate = 0.8f;     // d: predator deaths per predator per unit time
};

// Safety valve, not a modeling feature: the classic (no carrying-capacity) LV system
// is only neutrally stable, so demographic noise can occasionally drive predators
// extinct, after which prey growth is unopposed and would otherwise grow without
// bound. This caps per-tick births once a population gets absurdly large, so a rare
// bad draw degrades into flat growth instead of exhausting entt's entity storage.
inline constexpr std::size_t kMaxSpeciesPopulation = 20000;

} // namespace eco
