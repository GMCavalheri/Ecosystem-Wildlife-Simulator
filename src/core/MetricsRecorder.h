#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <entt/entt.hpp>

namespace eco {

class Grid;

// Records per-tick population/environment/trait stats for offline validation, e.g.
// confirming Lotka-Volterra-style oscillation (Phase 1), vegetation carrying capacity
// (Phase 2), or trait drift under selection pressure (Phase 3).
class MetricsRecorder {
public:
    struct Snapshot {
        float time = 0.0f;
        std::size_t preyCount = 0;
        std::size_t predatorCount = 0;
        float avgVegetation = 0.0f;
        float avgPreySpeed = 0.0f;
        float avgPredatorEnergy = 0.0f;
    };

    void snapshot(const entt::registry& registry, const Grid& grid, float time);

    const std::vector<Snapshot>& history() const { return history_; }

    // Writes time,prey,predator,avg_vegetation,avg_prey_speed,avg_predator_energy
    // rows for external plotting/validation.
    void writeCsv(const std::string& path) const;

private:
    std::vector<Snapshot> history_;
};

} // namespace eco
