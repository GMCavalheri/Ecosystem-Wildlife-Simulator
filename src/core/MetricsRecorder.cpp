#include "core/MetricsRecorder.h"

#include <fstream>

#include "components/Components.h"

namespace eco {

void MetricsRecorder::snapshot(const entt::registry& registry, float time) {
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

    history_.push_back(snap);
}

void MetricsRecorder::writeCsv(const std::string& path) const {
    std::ofstream out(path);
    out << "time,prey,predator\n";
    for (const auto& snap : history_) {
        out << snap.time << ',' << snap.preyCount << ',' << snap.predatorCount << '\n';
    }
}

} // namespace eco
