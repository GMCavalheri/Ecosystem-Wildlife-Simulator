#pragma once

#include <vector>

namespace eco {

// Terrain is a flat array, not ECS entities, for cache locality on grid-wide scans.
struct Cell {
    float vegetationDensity = 1.0f;
    float waterAvailability = 1.0f;
    float temperature = 20.0f;
};

class Grid {
public:
    Grid(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    Cell& at(int x, int y);
    const Cell& at(int x, int y) const;

private:
    int width_;
    int height_;
    std::vector<Cell> cells_;
};

} // namespace eco
