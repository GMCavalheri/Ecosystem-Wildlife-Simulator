#include "render/Viewer.h"

#include <algorithm>
#include <cstdio>

#include <raylib.h>

#include "components/Components.h"
#include "environment/Grid.h"

namespace eco {

namespace {

constexpr int kGridMarginX = 10;
constexpr int kGridMarginY = 10;
constexpr int kPanelX = 510;
constexpr int kPanelWidth = 330;
// HUD text in drawHud() below is 14 lines (22px each) plus 3 section gaps (8px each),
// starting at kGridMarginY, then a 20px-tall legend row -- kGraphY must clear all of
// that or the opaque graph panel drawn after it will paint over the tail of the HUD.
constexpr int kGraphY = kGridMarginY + 14 * 22 + 3 * 8 + 20 + 10;
constexpr int kGraphHeight = 260;

Color lerpColor(Color a, Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return Color{
        static_cast<unsigned char>(a.r + (b.r - a.r) * t),
        static_cast<unsigned char>(a.g + (b.g - a.g) * t),
        static_cast<unsigned char>(a.b + (b.b - a.b) * t),
        255,
    };
}

} // namespace

Viewer::Viewer() : sim_(gridWidth_, gridHeight_) { reset(); }

void Viewer::reset() {
    sim_ = Simulation(gridWidth_, gridHeight_);
    sim_.seedPopulation(kPreySpeciesId, 280);
    sim_.seedPopulation(kPredatorSpeciesId, 10);
    tickAccumulator_ = 0.0f;
}

void Viewer::run() {
    const int windowWidth = kPanelX + kPanelWidth + 10;
    const int gridPixelHeight = kGridMarginY + gridHeight_ * cellPixelSize_ + 10;
    const int panelPixelHeight = kGraphY + kGraphHeight + 10;
    const int windowHeight = std::max(gridPixelHeight, panelPixelHeight);
    InitWindow(windowWidth, windowHeight, "Ecosystem/Wildlife Simulator");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        handleInput();

        if (!paused_) {
            tickAccumulator_ += GetFrameTime() * ticksPerSecond_;
            constexpr float dt = 1.0f / 30.0f;
            while (tickAccumulator_ >= 1.0f) {
                sim_.tick(dt);
                tickAccumulator_ -= 1.0f;
            }
        }

        BeginDrawing();
        ClearBackground(Color{20, 20, 24, 255});
        drawGrid();
        drawAgents();
        drawHud();
        drawPopulationGraph();
        EndDrawing();
    }

    CloseWindow();
}

void Viewer::handleInput() {
    if (IsKeyPressed(KEY_SPACE)) {
        paused_ = !paused_;
    }
    if (IsKeyPressed(KEY_UP)) {
        ticksPerSecond_ = std::min(ticksPerSecond_ * 1.5f, 240.0f);
    }
    if (IsKeyPressed(KEY_DOWN)) {
        ticksPerSecond_ = std::max(ticksPerSecond_ / 1.5f, 1.0f);
    }
    if (IsKeyPressed(KEY_ONE)) {
        sim_.vegetationParams().regrowthRate =
            std::max(0.05f, sim_.vegetationParams().regrowthRate - 0.05f);
    }
    if (IsKeyPressed(KEY_TWO)) {
        sim_.vegetationParams().regrowthRate =
            std::min(2.0f, sim_.vegetationParams().regrowthRate + 0.05f);
    }
    if (IsKeyPressed(KEY_THREE)) {
        sim_.diseaseParams().transmissionRate =
            std::max(0.0f, sim_.diseaseParams().transmissionRate - 0.02f);
    }
    if (IsKeyPressed(KEY_FOUR)) {
        sim_.diseaseParams().transmissionRate =
            std::min(1.0f, sim_.diseaseParams().transmissionRate + 0.02f);
    }
    if (IsKeyPressed(KEY_FIVE)) {
        sim_.migrationParams().moveAttemptRate =
            std::max(0.0f, sim_.migrationParams().moveAttemptRate - 0.5f);
    }
    if (IsKeyPressed(KEY_SIX)) {
        sim_.migrationParams().moveAttemptRate =
            std::min(10.0f, sim_.migrationParams().moveAttemptRate + 0.5f);
    }
    if (IsKeyPressed(KEY_I)) {
        sim_.infectRandomPrey(20);
    }
    if (IsKeyPressed(KEY_R)) {
        reset();
    }
}

void Viewer::drawGrid() const {
    const Grid& grid = sim_.grid();
    constexpr Color dirt{101, 67, 33, 255};
    constexpr Color lush{34, 139, 34, 255};

    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const Color color = lerpColor(dirt, lush, grid.at(x, y).vegetationDensity);
            DrawRectangle(kGridMarginX + x * cellPixelSize_, kGridMarginY + y * cellPixelSize_,
                          cellPixelSize_, cellPixelSize_, color);
        }
    }
}

void Viewer::drawAgents() const {
    constexpr Color susceptibleColor{240, 240, 240, 230};
    constexpr Color infectedColor{220, 30, 30, 255};
    constexpr Color immuneColor{80, 200, 255, 230};

    const auto& registry = sim_.registry();
    auto view = registry.view<const Species, const Position>();
    const float radius = cellPixelSize_ * 0.3f;

    for (auto entity : view) {
        const SpeciesId speciesId = view.get<const Species>(entity).id;
        if (speciesId != kPreySpeciesId && speciesId != kCompetitorSpeciesId) {
            continue;
        }
        const auto& position = view.get<const Position>(entity);
        // Phase 8: the competitor herbivore is drawn amber when healthy.
        Color color = speciesId == kCompetitorSpeciesId ? Color{255, 190, 60, 235} : susceptibleColor;
        if (const auto* health = registry.try_get<const Health>(entity)) {
            if (health->infected) {
                color = infectedColor;
            } else if (health->immune) {
                color = immuneColor;
            }
        }
        const float px = kGridMarginX + (position.cellX + 0.5f) * cellPixelSize_;
        const float py = kGridMarginY + (position.cellY + 0.5f) * cellPixelSize_;
        DrawCircle(static_cast<int>(px), static_cast<int>(py), radius, color);
    }
}

void Viewer::drawHud() const {
    // Computed live from the registry every frame, not from the tick-based metrics
    // history -- that history only advances inside sim_.tick(), so while paused (e.g.
    // right after pressing 'I' to seed an outbreak) it would show stale counts even
    // though the grid rendering above already reflects the live state correctly.
    std::size_t preyCount = 0, predatorCount = 0, infectedCount = 0, immuneCount = 0;
    double speedTotal = 0.0;
    const auto& registry = sim_.registry();
    for (auto entity : registry.view<const Species>()) {
        const auto id = registry.get<const Species>(entity).id;
        if (id == kPreySpeciesId) {
            ++preyCount;
            if (const auto* traits = registry.try_get<const GeneticTraits>(entity)) {
                speedTotal += traits->speed;
            }
            if (const auto* health = registry.try_get<const Health>(entity)) {
                if (health->infected) {
                    ++infectedCount;
                } else if (health->immune) {
                    ++immuneCount;
                }
            }
        } else if (id == kPredatorSpeciesId) {
            ++predatorCount;
        }
    }
    double vegetationTotal = 0.0;
    const Grid& grid = sim_.grid();
    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            vegetationTotal += grid.at(x, y).vegetationDensity;
        }
    }
    const float avgVegetation =
        static_cast<float>(vegetationTotal / (grid.width() * grid.height()));
    const float avgSpeed = preyCount > 0 ? static_cast<float>(speedTotal / preyCount) : 0.0f;

    char line[128];
    int y = kGridMarginY;
    const Color textColor = RAYWHITE;

    auto draw = [&](const char* fmt, auto... args) {
        std::snprintf(line, sizeof(line), fmt, args...);
        DrawText(line, kPanelX, y, 18, textColor);
        y += 22;
    };

    draw("t = %.2f  %s", sim_.simulationTime(), paused_ ? "[PAUSED]" : "");
    draw("ticks/sec: %.1f", ticksPerSecond_);
    y += 8;
    draw("Prey:      %5zu", preyCount);
    draw("Predator:  %5zu", predatorCount);
    draw("Infected:  %5zu", infectedCount);
    draw("Immune:    %5zu", immuneCount);
    draw("Avg veg:   %.3f", avgVegetation);
    draw("Mean speed:%.3f", avgSpeed);
    y += 8;
    draw("[1/2] veg regrowth:   %.2f", sim_.vegetationParams().regrowthRate);
    draw("[3/4] disease beta:   %.2f", sim_.diseaseParams().transmissionRate);
    draw("[5/6] migration rate: %.2f", sim_.migrationParams().moveAttemptRate);
    y += 8;
    draw("[I] seed outbreak (20 prey)");
    draw("[SPACE] pause  [R] reset");
    draw("[UP/DOWN] sim speed");

    DrawText("Susceptible", kPanelX, y + 6, 14, Color{240, 240, 240, 230});
    DrawCircle(kPanelX + 100, y + 13, 5, Color{240, 240, 240, 230});
    DrawText("Infected", kPanelX + 130, y + 6, 14, Color{220, 30, 30, 255});
    DrawCircle(kPanelX + 200, y + 13, 5, Color{220, 30, 30, 255});
    DrawText("Immune", kPanelX + 230, y + 6, 14, Color{80, 200, 255, 230});
    DrawCircle(kPanelX + 285, y + 13, 5, Color{80, 200, 255, 230});
}

void Viewer::drawPopulationGraph() const {
    const int x0 = kPanelX;
    const int y0 = kGraphY;
    const int w = kPanelWidth;
    const int h = kGraphHeight;

    DrawRectangle(x0, y0, w, h, Color{30, 30, 36, 255});
    DrawRectangleLines(x0, y0, w, h, Color{90, 90, 100, 255});
    DrawText("Population (recent history)", x0 + 6, y0 + 4, 14, RAYWHITE);

    const auto& history = sim_.metrics().history();
    constexpr std::size_t kWindow = 600; // ~20 simulated time units of recent history
    const std::size_t start = history.size() > kWindow ? history.size() - kWindow : 0;
    if (history.size() - start < 2) {
        return;
    }

    std::size_t maxCount = 1;
    for (std::size_t i = start; i < history.size(); ++i) {
        maxCount = std::max({maxCount, history[i].preyCount, history[i].predatorCount});
    }

    const int plotTop = y0 + 24;
    const int plotBottom = y0 + h - 6;
    const int plotHeight = plotBottom - plotTop;
    const std::size_t count = history.size() - start;

    auto plot = [&](auto getter, Color color) {
        for (std::size_t i = start + 1; i < history.size(); ++i) {
            const float x1 =
                x0 + 6 + static_cast<float>(i - 1 - start) / (count - 1) * (w - 12);
            const float x2 = x0 + 6 + static_cast<float>(i - start) / (count - 1) * (w - 12);
            const float y1 = plotBottom - static_cast<float>(getter(history[i - 1])) /
                                               maxCount * plotHeight;
            const float y2 =
                plotBottom - static_cast<float>(getter(history[i])) / maxCount * plotHeight;
            DrawLineEx(Vector2{x1, y1}, Vector2{x2, y2}, 1.5f, color);
        }
    };

    plot([](const MetricsRecorder::Snapshot& s) { return s.preyCount; }, Color{60, 200, 60, 255});
    plot([](const MetricsRecorder::Snapshot& s) { return s.predatorCount; },
         Color{220, 60, 60, 255});
}

} // namespace eco
