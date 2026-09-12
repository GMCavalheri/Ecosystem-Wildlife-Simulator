#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <entt/entt.hpp>

namespace eco {

// Records per-tick population counts for offline validation, e.g. confirming
// Lotka-Volterra-style oscillation in Phase 1.
class MetricsRecorder {
public:
    struct Snapshot {
        float time = 0.0f;
        std::size_t preyCount = 0;
        std::size_t predatorCount = 0;
    };

    void snapshot(const entt::registry& registry, float time);

    const std::vector<Snapshot>& history() const { return history_; }

    // Writes time,prey,predator rows for external plotting/validation tools.
    void writeCsv(const std::string& path) const;

private:
    std::vector<Snapshot> history_;
};

} // namespace eco
