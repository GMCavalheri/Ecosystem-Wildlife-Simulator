#include "systems/MigrationSystem.h"

#include "environment/Grid.h"

namespace eco {

void MigrationSystem::update(entt::registry& /*registry*/, Grid& /*grid*/) {
    // Phase 5: move agents toward the neighboring cell with the best resource/threat gradient.
}

} // namespace eco
