#pragma once

#include <cstddef>

namespace eco {

// Safety valve, not a modeling feature: caps per-tick births once a population gets
// absurdly large, so a rare bad draw degrades into flat growth instead of exhausting
// entt's entity storage. See Phase 1 notes for how this was discovered.
inline constexpr std::size_t kMaxSpeciesPopulation = 20000;

// Starting Energy for prey and predators seeded at simulation setup.
inline constexpr float kInitialPreyEnergy = 50.0f;
inline constexpr float kInitialPredatorEnergy = 50.0f;

// Phase 2: per-cell vegetation regrowth. Logistic growth dV/dt = r(T)*V*(1-V), where
// the rate r(T) is scaled down by a Gaussian "suitability" curve around
// optimalTemperature -- vegetation grows fastest near the ideal temperature and
// stalls in a too-hot or too-cold season. Temperature itself cycles sinusoidally
// with simulated time to represent seasons.
struct VegetationParams {
    float regrowthRate = 0.4f;          // r: logistic growth rate at optimal temperature
    float baseTemperature = 20.0f;      // mean annual temperature
    float seasonalAmplitude = 10.0f;    // +/- swing around the mean over one year
    float seasonalPeriod = 40.0f;       // simulated time units per full seasonal cycle
    float optimalTemperature = 20.0f;   // temperature at which regrowth is fastest
    float temperatureTolerance = 15.0f; // sigma of the suitability Gaussian
    // Phase 8: degrees C from the top row to the bottom row (bottom is warmer when
    // positive), so the season shifts the band of fastest regrowth up and down the grid
    // -- the substrate for emergent seasonal migration. 0 = uniform climate.
    float latitudeGradient = 0.0f;
};

// Phase 2: herbivores eat vegetation from their own cell, convert it into Energy, and
// pay a constant metabolic cost just for staying alive. This is the real,
// space-limited carrying-capacity mechanism that Phase 1 lacked.
// A logistic patch's maximum *sustainable* yield is r/4 (harvested exactly at
// V=0.5, the peak of r*V*(1-V)) -- demand above that permanently drains the patch
// no matter how much slack it had. maxIntakeRate is deliberately kept just above
// that ceiling: a hungry prey can draw down a fresh patch quickly, but a fed one
// (throttled by energyRoom below) settles into a sustainable trickle.
struct ForagingParams {
    float maxIntakeRate = 0.12f;        // max vegetation units a prey can eat per unit time
    float energyPerVegetation = 400.0f; // energy gained per unit of vegetation eaten
    float metabolicRate = 10.0f;        // energy spent per unit time just staying alive
};

// Phase 2: prey reproduce once Energy crosses a threshold, at a constant probability
// rate per unit time, paying an energy cost to spawn an offspring at their own cell.
struct ReproductionParams {
    float energyThreshold = 70.0f;   // minimum Energy to be reproduction-eligible
    float attemptRate = 0.3f;        // probability rate per unit time once eligible
    float offspringEnergy = 40.0f;   // starting Energy given to the newborn
    float parentEnergyCost = 55.0f;  // Energy deducted from the parent per birth
};

// Phase 3: offspring GeneticTraits are the parent's plus independent Gaussian
// mutation, clamped to a sane positive range. Founders (seedPopulation) start
// homogeneous at 1.0 for every trait -- all variance seen later comes purely from
// mutation, so any trend in the population mean is a real, measurable response to
// selection pressure, not an artifact of the starting distribution.
struct GeneticsParams {
    float speedMutationStdDev = 0.05f;
    float sizeMutationStdDev = 0.05f;
    float fertilityMutationStdDev = 0.05f;
    float minTraitValue = 0.1f;
    float maxTraitValue = 3.0f;
};

// Phase 5: simple greedy gradient-following, not pathfinding. Each tick, an eligible
// prey looks at its 8 neighboring cells (Moore neighborhood) and moves to the best one
// if it beats its current cell's vegetation by more than minVegetationAdvantage --
// otherwise it stays put, so a flat gradient doesn't cause constant jitter.
struct MigrationParams {
    float moveAttemptRate = 2.0f;         // move attempts per unit time
    float minVegetationAdvantage = 0.05f; // required edge over the current cell to bother moving
};

// Predator resilience fix: predators now have their own Energy economy, mirroring
// prey's Phase 2 mechanic, instead of a flat background death rate with zero buffer.
// A kill's prey biomass feeds ONE randomly-chosen existing predator's Energy (not an
// instant population-wide birth), predators pay a constant metabolic cost every tick
// regardless of hunting success, starve if Energy hits zero, and reproduce
// individually once Energy crosses a threshold -- the same "coast through a lean
// patch" resilience prey have always had, instead of instant, unbuffered dependence
// on that exact tick's kill count. Diagnosed via tracing: with the old flat
// Poisson(d*Predator) death and no reserves, small predator populations kept landing
// exactly on zero -- an absorbing state, since a kill (and so a new predator) requires
// at least one living predator to begin with.
struct PredatorParams {
    float predationRate = 0.002f;         // b: predation events per prey-predator pair per unit time
    float energyPerKill = 200.0f;         // Energy gained by the predator that makes a kill
    float metabolicRate = 15.0f;          // Energy spent per unit time just staying alive
    float reproductionThreshold = 70.0f;  // minimum Energy to be reproduction-eligible
    float reproductionAttemptRate = 0.2f; // probability rate per unit time once eligible
    float offspringEnergy = 40.0f;        // starting Energy given to the newborn
    float parentEnergyCost = 55.0f;       // Energy deducted from the parent per birth
};

// Phase 4: SIR-style disease, local rather than mean-field -- transmission only
// happens between prey sharing the same cell, so infection probability scales with
// local crowding (contact rate), exactly the "self-correcting overcrowding" the plan
// calls for. An infected individual leaves the infectious state at combined rate
// (recoveryRate + diseaseDeathRate); which of the two happens is decided by their
// relative share of that combined rate -- the standard competing-exponential-hazards
// construction, so case fatality is a derived quantity, not a separately hand-tuned
// parameter. R0 = transmissionRate / (recoveryRate + diseaseDeathRate) in the
// well-mixed (single-cell) special case used for validation.
struct DiseaseParams {
    float transmissionRate = 0.15f;  // beta: infection probability rate per infected cellmate per unit time
    float recoveryRate = 0.3f;       // gamma: recovery rate per unit time while infected
    float diseaseDeathRate = 0.05f;  // mu: disease-induced death rate per unit time while infected
};

} // namespace eco
