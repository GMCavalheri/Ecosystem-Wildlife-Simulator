#pragma once

#include <cstdint>

namespace eco {

using SpeciesId = std::uint32_t;

// Phase 1: the two species in the mean-field Lotka-Volterra validation.
inline constexpr SpeciesId kPreySpeciesId = 0;
inline constexpr SpeciesId kPredatorSpeciesId = 1;

struct Position {
    int cellX = 0;
    int cellY = 0;
};

struct Species {
    SpeciesId id = 0;
};

// Hunger / fat reserves.
struct Energy {
    float value = 0.0f;
    float max = 100.0f;
};

struct Age {
    float years = 0.0f;
};

// Small per-individual variance, mutated on reproduction (Phase 3).
struct GeneticTraits {
    float speed = 1.0f;
    float size = 1.0f;
    float fertility = 1.0f;
};

// value is a reserved hook for a future phase (e.g. combat/environmental damage) --
// Phase 4's SIR disease model only reads/writes infected/immune/infectionTimer.
struct Health {
    float value = 100.0f;
    bool infected = false;
    float infectionTimer = 0.0f;
    bool immune = false; // recovered -- the "R" compartment in SIR
};

// Crosses a threshold to spawn offspring.
struct Reproductive {
    float readiness = 0.0f;
};

} // namespace eco
