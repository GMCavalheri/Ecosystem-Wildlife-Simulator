#include "core/MetricsRecorder.h"

#include <fstream>

#include "components/Components.h"
#include "components/PreyGroup.h"
#include "environment/Grid.h"

namespace eco {

void MetricsRecorder::snapshot(const entt::registry& registry, const Grid& grid, float time) {
    Snapshot snap;
    snap.time = time;

    double speedTotal = 0.0;
    double predatorEnergyTotal = 0.0;
    if (auto prey = preyGroup(registry)) {
        for (auto [entity, position, energy, traits, health] : prey.each()) {
            ++snap.preyCount;
            speedTotal += traits.speed;
            if (health.infected) {
                ++snap.infectedPreyCount;
            } else if (health.immune) {
                ++snap.immunePreyCount;
            }
        }
    }
    for (auto [entity, species, energy] :
         registry.view<const Species, const Energy>(entt::exclude<GeneticTraits>).each()) {
        if (species.id == kPredatorSpeciesId) {
            ++snap.predatorCount;
            predatorEnergyTotal += energy.value;
        }
    }
    snap.avgPreySpeed =
        snap.preyCount > 0 ? static_cast<float>(speedTotal / snap.preyCount) : 0.0f;
    snap.avgPredatorEnergy =
        snap.predatorCount > 0 ? static_cast<float>(predatorEnergyTotal / snap.predatorCount) : 0.0f;

    double vegetationTotal = 0.0;
    const int cellCount = grid.width() * grid.height();
    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            vegetationTotal += grid.at(x, y).vegetationDensity;
        }
    }
    snap.avgVegetation = cellCount > 0 ? static_cast<float>(vegetationTotal / cellCount) : 0.0f;

    history_.push_back(snap);
}

void MetricsRecorder::writeCsv(const std::string& path) const {
    std::ofstream out(path);
    out << "time,prey,predator,avg_vegetation,avg_prey_speed,avg_predator_energy,"
           "infected_prey,immune_prey\n";
    for (const auto& snap : history_) {
        out << snap.time << ',' << snap.preyCount << ',' << snap.predatorCount << ','
            << snap.avgVegetation << ',' << snap.avgPreySpeed << ',' << snap.avgPredatorEnergy
            << ',' << snap.infectedPreyCount << ',' << snap.immunePreyCount << '\n';
    }
}

} // namespace eco
