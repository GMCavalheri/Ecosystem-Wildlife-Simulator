#pragma once

#include <entt/entt.hpp>

namespace eco {

// Records per-tick state (e.g. population counts) for offline validation, such as
// confirming Lotka-Volterra-style oscillation in Phase 1.
class MetricsRecorder {
public:
    void snapshot(const entt::registry& registry);
};

} // namespace eco
