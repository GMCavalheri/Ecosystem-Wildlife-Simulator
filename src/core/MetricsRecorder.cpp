#include "core/MetricsRecorder.h"

#include <fstream>

#include "components/Components.h"
#include "environment/Grid.h"

namespace eco {

void MetricsRecorder::snapshot(const entt::registry& registry, const Grid& grid, float time) {
    Snapshot snap;
    snap.time = time;

    auto view = registry.view<const Species>();
    for (auto entity : view) {
        const auto id = view.get<const Species>(entity).id;
        if (id == kPreySpeciesId) {
            ++snap.preyCount;
        } else if (id == kPredatorSpeciesId) {
            ++snap.predatorCount;
        }
    }

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
    out << "time,prey,predator,avg_vegetation\n";
    for (const auto& snap : history_) {
        out << snap.time << ',' << snap.preyCount << ',' << snap.predatorCount << ','
            << snap.avgVegetation << '\n';
    }
}

} // namespace eco
