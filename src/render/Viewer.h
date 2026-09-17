#pragma once

#include "core/Simulation.h"

namespace eco {

// Phase 6: a raylib-based top-down grid viewer with a live HUD, population graph, and
// keyboard-tunable parameters. This is a thin, optional layer on top of the headless
// Simulation -- only built when the vcpkg "render" feature is enabled (see
// vcpkg.json); the core simulation has no idea this exists.
class Viewer {
public:
    Viewer();

    // Runs the window loop until the user closes it. Blocks until then.
    void run();

private:
    void handleInput();
    void drawGrid() const;
    void drawAgents() const;
    void drawHud() const;
    void drawPopulationGraph() const;
    void reset();

    // Declared before sim_ so they're initialized first -- Viewer's constructor uses
    // them to size the Simulation.
    int cellPixelSize_ = 10;
    int gridWidth_ = 48;
    int gridHeight_ = 48;

    Simulation sim_;
    float ticksPerSecond_ = 30.0f;
    float tickAccumulator_ = 0.0f;
    bool paused_ = false;
};

} // namespace eco
