#include <cstddef>

#include <catch2/catch_test_macros.hpp>

#include "components/Components.h"
#include "core/Simulation.h"

namespace {

enum class Outcome { CompetitorOutlasts, PreyOutlasts, Tie };

// Two herbivore species share one vegetation supply (no predators, no disease). Without
// predators the shared resource is eventually exhausted and both collapse (the Phase 5
// tragedy of the commons), so the meaningful question is which species is still alive
// last: returns who outlasted whom. The competitor differs from the original prey only
// in its metabolic cost.
Outcome whoOutlasts(float competitorMetabolicRate, unsigned seed) {
    eco::PredatorParams noPredators;
    noPredators.predationRate = 0.0f;
    eco::Simulation sim(32, 32, noPredators, {}, {}, {}, {}, {}, {}, seed);

    eco::ForagingParams competitorForaging;
    competitorForaging.metabolicRate = competitorMetabolicRate;
    sim.setCompetitorParams(competitorForaging, {});

    sim.seedPopulation(eco::kPreySpeciesId, 75);
    sim.seedPopulation(eco::kCompetitorSpeciesId, 75);

    long preyLastAlive = 0, competitorLastAlive = 0;
    for (long tick = 1; tick <= 30 * 70; ++tick) {
        sim.tick(1.0f / 30.0f);
        const auto& s = sim.metrics().history().back();
        if (s.preyCount > 0) preyLastAlive = tick;
        if (s.competitorCount > 0) competitorLastAlive = tick;
        if (s.preyCount == 0 && s.competitorCount == 0) break;
    }
    if (competitorLastAlive > preyLastAlive) return Outcome::CompetitorOutlasts;
    if (competitorLastAlive < preyLastAlive) return Outcome::PreyOutlasts;
    return Outcome::Tie;
}

struct Tally {
    int competitorWins = 0;
    int preyWins = 0;
};

Tally tally(float competitorMetabolicRate, int seeds) {
    Tally result;
    for (unsigned seed = 1; seed <= static_cast<unsigned>(seeds); ++seed) {
        switch (whoOutlasts(competitorMetabolicRate, seed)) {
        case Outcome::CompetitorOutlasts: ++result.competitorWins; break;
        case Outcome::PreyOutlasts: ++result.preyWins; break;
        case Outcome::Tie: break;
        }
    }
    return result;
}

} // namespace

// Phase 8 correctness gate for "more species": competitive exclusion (Gause's principle).
// Two species living off the same resource cannot stably coexist; the one that can get by
// on less wins. Here "less" is a lower metabolic cost, and nothing tells the animals who
// should win -- it falls out of the foraging/energy/reproduction rules.
//
// The identical-species control is what makes the result trustworthy: with equal
// parameters the outcome must be a fair coin (a bias toward one species id would show up
// here), and the reversed case (a *worse* competitor) must flip the result.
// Measured on 20 seeds: control 11/9, competitor at metabolism 9 (10% cheaper) 20/0.
TEST_CASE("The more efficient herbivore species outcompetes the other, and identical "
          "species split evenly",
          "[competition]") {
    constexpr int seeds = 14;

    // Control: identical species -> coin flip. Binomial(14, 0.5) essentially never lands
    // outside [2, 12]; a built-in bias would.
    const Tally control = tally(10.0f, seeds);
    REQUIRE(control.competitorWins >= 2);
    REQUIRE(control.competitorWins <= 12);

    // Treatment: a 10% cheaper metabolism -> the competitor outlasts in nearly every run
    // (P(>=12 of 14 by chance) < 1%).
    const Tally fitter = tally(9.0f, seeds);
    REQUIRE(fitter.competitorWins >= 12);

    // Reverse: a 20% costlier metabolism -> the original prey outlast.
    const Tally worse = tally(12.0f, seeds);
    REQUIRE(worse.preyWins >= 12);
}

TEST_CASE("Offspring inherit their parent's species and both species are counted separately",
          "[competition]") {
    eco::PredatorParams noPredators;
    noPredators.predationRate = 0.0f;
    eco::Simulation sim(32, 32, noPredators, {}, {}, {}, {}, {}, {}, 5u);
    sim.seedPopulation(eco::kCompetitorSpeciesId, 60);
    // Only the competitor is seeded, so any original-prey individual could only appear if
    // offspring were mis-labelled.
    for (int i = 0; i < 150; ++i) sim.tick(1.0f / 30.0f);

    const auto& s = sim.metrics().history().back();
    REQUIRE(s.competitorCount > 60); // it reproduced...
    REQUIRE(s.preyCount == 0);       // ...and every newborn is still a competitor
}
